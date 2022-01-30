
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>
#include "smemlib.h"
#include <limits.h>
#include <semaphore.h>

// Define a name for your shared memory; you can give any name that start with a slash character; it will be like a filename.
char sharedMemoryName[] = "/sharedSegmentName";

// Define semaphore(s)
sem_t mutex;

// Define your stuctures and variables.
int shm_fd;
int processCount = 0;
int sharedMemorySize = -1;
int memoryUsed = -1; // indicates if the allocation is made for the first time, this will insert first header!

void *(*allocationAlgo)(int, void *);

const int ALLOCATION_LENGTH_OFFSET = 13;
const int ALLOCATION_USED_OFFSET = 12;
const int ALLOCATION_PID_OFFSET = 8;
const int ALLOCATION_PREV_OFFSET = 4;
const int HEADER_SIZE = 17;

const int MAX_SEGMENT_SIZE = (1 << 20) * 4;
const int MIN_SEGMENT_SIZE = (1 << 10) * 32;
const int MAX_PROCESS_COUNT = 10;

struct ProcessDatum
{
    pid_t processID;
    void *ptr;
};
struct ProcessDatum processData[10];
signed char usedData[10] = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1};

int smem_init(int segmentsize)
{
    // validate segment size
    if (segmentsize < MIN_SEGMENT_SIZE || segmentsize > MAX_SEGMENT_SIZE)
    {
        return -1;
    }

    // check for power of 2!
    int powerChecker = 1;
    while (powerChecker < segmentsize)
    {
        powerChecker = powerChecker << 1;
    }
    if (powerChecker != segmentsize)
    {
        return -1;
    }

    shm_fd = shm_open(sharedMemoryName, O_CREAT | O_EXCL | O_RDWR, 0666);
    if (shm_fd < 0)
    {
        if (errno == EEXIST) 
        {
            shm_unlink(sharedMemoryName);
            shm_fd = shm_open(sharedMemoryName, O_CREAT | O_EXCL | O_RDWR, 0666);
        }
        else
        {
            return -1;
        }
    }

    if (shm_fd >= 0)
    {
        int truncateRes = ftruncate(shm_fd, segmentsize);
        if (truncateRes < 0)
        {
            return -1;
        }
        sharedMemorySize = segmentsize;
        allocationAlgo = &smem_firstFit;

        // Initialize semaphore(s)
        sem_init(&mutex, 1, 1); // First 1 indicates that it is used between processes, second 1 is the initial value
    }
    return shm_fd >= 0 ? 0 : -1;
}

int smem_remove()
{
    // also reset process count, semephores, any data, make everything reset! should be reubsale once called init!
    processCount = 0;
    for (int i = 0; i < MAX_PROCESS_COUNT; i++)
    {
        usedData[i] = -1;
    }
    sharedMemorySize = -1;
    memoryUsed = -1;

    // Deinitialize semephore
    sem_destroy(&mutex);

    return shm_unlink(sharedMemoryName) >= 0 ? 0 : -1;
}

int smem_open()
{
    //To avoid from race condition
    sem_wait(&mutex);

    if (sharedMemorySize < 0)
    {
        sem_post(&mutex);

        return -1;
    }

    // limit the usage of the library!
    if (processCount < MAX_PROCESS_COUNT)
    {
        // find the available spot!
        for (int i = 0; i < MAX_PROCESS_COUNT; i++)
        {
            if (usedData[i] < 0)
            {
                // map the memory
                processData[i].processID = getpid();
                processData[i].ptr = mmap(NULL, sharedMemorySize, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);

                if (processData[i].ptr != MAP_FAILED)
                {
                    // allocate this slot to the requesting process
                    usedData[i] = 1;
                    processCount++;

                    sem_post(&mutex);

                    return 0;
                }
                sem_post(&mutex);

                return -1;
            }
        }
    }

    sem_post(&mutex);

    return -1;
}

void *smem_alloc(int size)
{
    //To avoid from race condition
    sem_wait(&mutex);

    // get the memory ptr of the process
    pid_t requestingProcessID = getpid();
    void *processMemoryPtr = NULL;
    // check if process id is valid
    for (int i = 0; i < MAX_PROCESS_COUNT; i++)
    {
        if (usedData[i] > 0 && processData[i].processID == requestingProcessID)
        {
            processMemoryPtr = processData[i].ptr;
            break;
        }
    }
    if (processMemoryPtr == NULL) //check permission
    {
        sem_post(&mutex);

        return NULL;
    }

    // can allocate a multiple of 8 bytes
    if (size % 8 != 0)
    {
        size = ((size / 8) + 1) * 8;
    }

    //If memory is not used before, initialize the first header.
    if (memoryUsed < 0)
    {
        *((int *)processMemoryPtr) = -1;
        *((int *)processMemoryPtr + ALLOCATION_PREV_OFFSET) = -1;
        memoryUsed = 1;
    }

    void *ptr = allocationAlgo(size, processMemoryPtr);

    sem_post(&mutex);

    return ptr;
}

void smem_free(void *p)
{
    //To avoid from race condition
    sem_wait(&mutex);

    smem_library_free(p);

    sem_post(&mutex);
}

pid_t closeingProcess;
int smem_close()
{
    //To avoid from race condition
    sem_wait(&mutex);
    closeingProcess = getpid();

    if (sharedMemorySize < 0)
    {
        sem_post(&mutex);

        return -1;
    }

    // find the process
    pid_t processIDToClose = getpid();
    for (int i = 0; i < MAX_PROCESS_COUNT; i++)
    {
        if (usedData[i] > 0 && processData[i].processID == processIDToClose)
        {
            //deallocate every memory used by the process
            void *curHead = processData[i].ptr;
            void *prevHead = NULL;
            while (*((int *)(curHead)) > 0)
            {
                if (*((pid_t *)(curHead + ALLOCATION_PID_OFFSET)) == processIDToClose)
                {
                    prevHead = curHead;
                }
                curHead += *((int *)(curHead));
                if (prevHead != NULL)
                {
                    smem_library_free(prevHead + HEADER_SIZE);
                    prevHead = NULL;
                }
            }

            // unmap the memory
            int unmapRes = munmap(processData[i].ptr, sharedMemorySize);

            // check unmap result
            if (unmapRes >= 0)
            {
                usedData[i] = -1;
                processCount--;

                sem_post(&mutex);

                return 0;
            }
            sem_post(&mutex);

            return -1;
        }
    }
    sem_post(&mutex);

    return -1;
}

void smem_library_free(void *p)
{
    // get the memory ptr of the process
    pid_t requestingProcessID = getpid();
    void *processMemoryPtr = NULL;
    // check if process id is valid
    for (int i = 0; i < MAX_PROCESS_COUNT; i++)
    {
        if (processData[i].processID == requestingProcessID)
        {
            processMemoryPtr = processData[i].ptr;
        }
    }
    if (processMemoryPtr == NULL) //check permission
    {

        return;
    }

    void *headPtr = p - HEADER_SIZE;

    // scenario 1 - at the end of the list
    //DONE
    if (((headPtr - processMemoryPtr) + HEADER_SIZE + *((int *)(headPtr + ALLOCATION_LENGTH_OFFSET))) == sharedMemorySize || *((int *)(headPtr + *((int *)(headPtr)))) < 0)
    {
        *((int *)(headPtr)) = -1;
        if (headPtr - processMemoryPtr != 0) // check if deallocation is not made at the first index that is eual to last index => single node.
        {
            void *prevHeaderPtr = headPtr - *((int *)(headPtr + ALLOCATION_PREV_OFFSET));
            if (*((char *)(prevHeaderPtr + ALLOCATION_USED_OFFSET)) < 0)
            {
                *((int *)prevHeaderPtr) = -1;
            }
        }
        return;
    }

    // scneraio 2 - at the beginning of the list
    //DONE
    if (headPtr - processMemoryPtr == 0)
    {
        *((char *)(headPtr + ALLOCATION_USED_OFFSET)) = -1;
        void *nextHeaderPtr = headPtr + *((int *)(headPtr));
        if (*((int *)(nextHeaderPtr)) < 0) // next ptr is end of the list
        {
            *((int *)(headPtr)) = -1;
        }
        else if (*((char *)(nextHeaderPtr + ALLOCATION_USED_OFFSET)) < 0) // next ptr is not used, merge
        {
            void *usedMemoryPtr = nextHeaderPtr + *((int *)(nextHeaderPtr));
            *((int *)(headPtr)) = usedMemoryPtr - headPtr;
            *((int *)(headPtr + ALLOCATION_USED_OFFSET)) = -1;
            *((int *)(headPtr + ALLOCATION_LENGTH_OFFSET)) = usedMemoryPtr - (headPtr + HEADER_SIZE);

            *((int *)(usedMemoryPtr + ALLOCATION_PREV_OFFSET)) = usedMemoryPtr - headPtr;
        }
        else // next ptr is used
        {
            *((int *)(headPtr + ALLOCATION_USED_OFFSET)) = -1;
            *((int *)(headPtr + ALLOCATION_LENGTH_OFFSET)) = nextHeaderPtr - (headPtr + HEADER_SIZE);
        }
        return;
    }

    // scenario 3 - in the middle
    //DONE?
    void *prevHeaderPtr = headPtr - *((int *)(headPtr + ALLOCATION_PREV_OFFSET));
    void *nextHeaderPtr = headPtr + *((int *)(headPtr));
    char isPrevUsed = *((char *)(prevHeaderPtr + ALLOCATION_USED_OFFSET));
    char isNextUsed = *((char *)(nextHeaderPtr + ALLOCATION_USED_OFFSET));
    if (isPrevUsed < 0 && isNextUsed < 0) //      Scenario 3.1 both prior and next are empty
    {
        void *usedNextPtr = nextHeaderPtr + *((int *)(nextHeaderPtr));
        *((int *)(prevHeaderPtr)) = usedNextPtr - prevHeaderPtr;
        *((int *)(usedNextPtr + ALLOCATION_PREV_OFFSET)) = usedNextPtr - prevHeaderPtr;
        *((int *)(prevHeaderPtr + ALLOCATION_LENGTH_OFFSET)) = usedNextPtr - (prevHeaderPtr + HEADER_SIZE);
    }
    else if (isNextUsed < 0) //      Scenario 3.2 only next is empty
    {
        void *usedNextPtr = nextHeaderPtr + *((int *)(nextHeaderPtr));
        *((char *)(headPtr + ALLOCATION_USED_OFFSET)) = -1;
        *((int *)(headPtr)) = usedNextPtr - headPtr;
        *((int *)(usedNextPtr + ALLOCATION_PREV_OFFSET)) = usedNextPtr - headPtr;
        *((int *)(headPtr + ALLOCATION_LENGTH_OFFSET)) = usedNextPtr - (headPtr + HEADER_SIZE);
    }
    else if (isPrevUsed < 0) //      Scenario 3.3 only prior is empty
    {
        *((int *)(prevHeaderPtr)) = nextHeaderPtr - prevHeaderPtr;
        *((int *)(prevHeaderPtr + ALLOCATION_LENGTH_OFFSET)) = nextHeaderPtr - (prevHeaderPtr + HEADER_SIZE);
        *((int *)(nextHeaderPtr + ALLOCATION_PREV_OFFSET)) = nextHeaderPtr - prevHeaderPtr;
    }
    else //      Scenario 3.4 they are full
    {
        *((char *)(headPtr + ALLOCATION_USED_OFFSET)) = -1;
        *((int *)(headPtr + ALLOCATION_LENGTH_OFFSET)) = nextHeaderPtr - (headPtr + HEADER_SIZE);
    }
}

//

//

//

/// Custom functions

int smem_get_mem_utilization(int *totUnusedSize) // for experiment, gets total number of unused blocks and totoal unused byte count
{
    //To avoid from race condition
    sem_wait(&mutex);

    // get the memory ptr of the process
    pid_t requestingProcessID = getpid();
    void *processMemoryPtr = NULL;
    // check if process id is valid
    for (int i = 0; i < MAX_PROCESS_COUNT; i++)
    {
        if (usedData[i] > 0 && processData[i].processID == requestingProcessID)
        {
            processMemoryPtr = processData[i].ptr;
            break;
        }
    }
    if (processMemoryPtr == NULL) //check permission
    {
        sem_post(&mutex);
        *totUnusedSize = -1;
        return -1;
    }

    //If memory is not used before, initialize the first header.
    if (memoryUsed < 0)
    {
        sem_post(&mutex);
        *totUnusedSize = -1;
        return -1;
    }

    void *curMemPointer = processMemoryPtr;
    int unusedSizeCount = 0;
    int unusedBlockCount = 0;
    while (*((int *)curMemPointer) > 0)
    {
        // current in between hole can be allocated to the requesting process
        if (*((char *)(curMemPointer + ALLOCATION_USED_OFFSET)) < 0)
        {
            unusedBlockCount++;
            unusedSizeCount += *((int *)(curMemPointer + ALLOCATION_LENGTH_OFFSET));
        }
        curMemPointer += *((int *)(curMemPointer));
    }
    sem_post(&mutex);
    *totUnusedSize = unusedSizeCount;
    return unusedBlockCount;
}

void *smem_firstFit(int size, void *shmPtr)
{
    //find a suitable spot
    void *foundAddress = NULL;
    void *curMemPointer = shmPtr;
    void *prevAddress = NULL;
    while (*((int *)curMemPointer) > 0)
    {
        // current in between hole can be allocated to the requesting process
        if (*((char *)(curMemPointer + ALLOCATION_USED_OFFSET)) < 0 &&
            *((int *)(curMemPointer + ALLOCATION_LENGTH_OFFSET)) >= size)
        {
            foundAddress = curMemPointer + HEADER_SIZE;
            int remSizeInHole = *((int *)(curMemPointer + ALLOCATION_LENGTH_OFFSET)) - size;

            *((char *)(curMemPointer + ALLOCATION_USED_OFFSET)) = 1;
            *((pid_t *)(curMemPointer + ALLOCATION_PID_OFFSET)) = getpid();
            *((int *)(curMemPointer + ALLOCATION_LENGTH_OFFSET)) = size;
            *((int *)(curMemPointer + ALLOCATION_PREV_OFFSET)) = curMemPointer - prevAddress;

            // There is enough space left for a small hole, handle connection
            if (remSizeInHole > HEADER_SIZE)
            {
                // connect handle
                void *newHoleAddress = foundAddress + size;
                *((int *)(newHoleAddress)) = curMemPointer + *((int *)(curMemPointer)) - newHoleAddress;
                *((int *)(curMemPointer)) = newHoleAddress - curMemPointer;
                *((int *)(newHoleAddress + ALLOCATION_PREV_OFFSET)) = newHoleAddress - curMemPointer;

                void *nextAddressPtr = newHoleAddress + *((int *)(newHoleAddress));
                *((int *)(nextAddressPtr + ALLOCATION_PREV_OFFSET)) = nextAddressPtr - newHoleAddress;

                //set settings of remaining hole
                *((char *)(newHoleAddress + ALLOCATION_USED_OFFSET)) = -1;
                *((int *)(newHoleAddress + ALLOCATION_LENGTH_OFFSET)) = remSizeInHole;
            }
            return foundAddress;
        }
        prevAddress = curMemPointer;
        curMemPointer += *((int *)(curMemPointer));
    }

    //a hole could not be found, allocate at the end of tail if there is memory
    if (sharedMemorySize - (curMemPointer - shmPtr) > HEADER_SIZE + size)
    {
        // allocate memory
        foundAddress = curMemPointer + HEADER_SIZE;
        *((char *)(curMemPointer + ALLOCATION_USED_OFFSET)) = 1;
        *((pid_t *)(curMemPointer + ALLOCATION_PID_OFFSET)) = getpid();
        *((int *)(curMemPointer + ALLOCATION_LENGTH_OFFSET)) = size;
        if (prevAddress != NULL)
        {
            *((int *)(curMemPointer + ALLOCATION_PREV_OFFSET)) = curMemPointer - prevAddress;
        }
        else
        {
            *((int *)(curMemPointer + ALLOCATION_PREV_OFFSET)) = -1;
        }

        // create new end tail
        void *tailHoleAddress = foundAddress + size;
        *((int *)(tailHoleAddress)) = -1;
        *((int *)(tailHoleAddress + ALLOCATION_PREV_OFFSET)) = tailHoleAddress - curMemPointer;
        *((int *)(curMemPointer)) = tailHoleAddress - curMemPointer;
    }
    return foundAddress;
}

void *smem_bestFit(int size, void *shmPtr)
{
    void *bestFitAddress = NULL;
    void *bestFitPrevAddress = NULL;
    void *curMemPointer = shmPtr;
    void *prevAddress = NULL;
    int minSizeFound = INT_MAX;
    while (*((int *)curMemPointer) > 0)
    {
        int sizeInBlock = *((int *)(curMemPointer + ALLOCATION_LENGTH_OFFSET));
        // current in between hole can be allocated to the requesting process
        if (*((char *)(curMemPointer + ALLOCATION_USED_OFFSET)) < 0 &&
            sizeInBlock >= size)
        {
            // check size
            if (minSizeFound > sizeInBlock)
            {
                minSizeFound = sizeInBlock;
                bestFitPrevAddress = prevAddress;
                bestFitAddress = curMemPointer;
                if (minSizeFound - size == 0) // actually the best fit possible, return immediately
                {
                    *((char *)(curMemPointer + ALLOCATION_USED_OFFSET)) = 1;
                    *((pid_t *)(curMemPointer + ALLOCATION_PID_OFFSET)) = getpid();
                    *((int *)(curMemPointer + ALLOCATION_LENGTH_OFFSET)) = size;
                    if (curMemPointer - shmPtr != 0)
                    {
                        *((int *)(curMemPointer + ALLOCATION_PREV_OFFSET)) = curMemPointer - prevAddress;
                    }
                    else
                    {
                        *((int *)(curMemPointer + ALLOCATION_PREV_OFFSET)) = -1;
                    }
                    return bestFitAddress + HEADER_SIZE;
                }
            }
        }
        prevAddress = curMemPointer;
        curMemPointer += *((int *)(curMemPointer));
    }

    // check if there is enough memory at the end
    if (sharedMemorySize - (curMemPointer - shmPtr) > HEADER_SIZE + size)
    {
        // best fit could not be found, try to insert at the end
        if (bestFitAddress == NULL)
        {
            // allocate memory
            bestFitAddress = curMemPointer + HEADER_SIZE;
            *((char *)(curMemPointer + ALLOCATION_USED_OFFSET)) = 1;
            *((pid_t *)(curMemPointer + ALLOCATION_PID_OFFSET)) = getpid();
            *((int *)(curMemPointer + ALLOCATION_LENGTH_OFFSET)) = size;
            if (curMemPointer - shmPtr != 0)
            {
                *((int *)(curMemPointer + ALLOCATION_PREV_OFFSET)) = curMemPointer - prevAddress;
            }
            else
            {
                *((int *)(curMemPointer + ALLOCATION_PREV_OFFSET)) = -1;
            }

            // create new end tail
            void *tailHoleAddress = bestFitAddress + size;
            *((int *)(tailHoleAddress)) = -1;
            *((int *)(tailHoleAddress + ALLOCATION_PREV_OFFSET)) = tailHoleAddress - curMemPointer;
            *((int *)(curMemPointer)) = tailHoleAddress - curMemPointer;
            return bestFitAddress;
        }
        else // compare remaining memory and best fit
        {
            int remMemSize = sharedMemorySize - ((curMemPointer - shmPtr) + HEADER_SIZE);
            if (remMemSize < minSizeFound && remMemSize >= size) // insert it end the tail
            {
                // allocate memory
                bestFitAddress = curMemPointer + HEADER_SIZE;
                *((char *)(curMemPointer + ALLOCATION_USED_OFFSET)) = 1;
                *((pid_t *)(curMemPointer + ALLOCATION_PID_OFFSET)) = getpid();
                *((int *)(curMemPointer + ALLOCATION_LENGTH_OFFSET)) = size;
                if (curMemPointer - shmPtr != 0)
                {
                    *((int *)(curMemPointer + ALLOCATION_PREV_OFFSET)) = curMemPointer - prevAddress;
                }
                else
                {
                    *((int *)(curMemPointer + ALLOCATION_PREV_OFFSET)) = -1;
                }

                // create new end tail
                void *tailHoleAddress = bestFitAddress + size;
                *((int *)(tailHoleAddress)) = -1;
                *((int *)(tailHoleAddress + ALLOCATION_PREV_OFFSET)) = tailHoleAddress - curMemPointer;
                *((int *)(curMemPointer)) = tailHoleAddress - curMemPointer;
            }
            else // insert it at the bestAddressFiund
            {
                *((char *)(bestFitAddress + ALLOCATION_USED_OFFSET)) = 1;
                *((pid_t *)(bestFitAddress + ALLOCATION_PID_OFFSET)) = getpid();
                *((int *)(bestFitAddress + ALLOCATION_LENGTH_OFFSET)) = size;
                *((int *)(bestFitAddress + ALLOCATION_PREV_OFFSET)) = bestFitAddress - bestFitPrevAddress;
                int remSizeInHole = *((int *)(bestFitAddress + ALLOCATION_LENGTH_OFFSET)) - size;
                // There is enough space left for a small hole, handle connection
                if (remSizeInHole > HEADER_SIZE)
                {
                    // connect handle
                    void *newHoleAddress = bestFitAddress + size + HEADER_SIZE;
                    *((int *)(newHoleAddress)) = bestFitAddress + *((int *)(bestFitAddress)) - newHoleAddress;
                    *((int *)(bestFitAddress)) = newHoleAddress - bestFitAddress;
                    *((int *)(newHoleAddress + ALLOCATION_PREV_OFFSET)) = newHoleAddress - bestFitAddress;

                    void *nextAddressPtr = newHoleAddress + *((int *)(newHoleAddress));
                    *((int *)(nextAddressPtr + ALLOCATION_PREV_OFFSET)) = nextAddressPtr - newHoleAddress;

                    //set settings of remaining hole
                    *((char *)(newHoleAddress + ALLOCATION_USED_OFFSET)) = -1;
                    *((int *)(newHoleAddress + ALLOCATION_LENGTH_OFFSET)) = remSizeInHole;
                }
                bestFitAddress = bestFitAddress + HEADER_SIZE;
            }
            return bestFitAddress;
        }
    }
    else if (bestFitAddress != NULL) // not enough memory left, chec best address found to insert here
    {
        *((char *)(bestFitAddress + ALLOCATION_USED_OFFSET)) = 1;
        *((pid_t *)(bestFitAddress + ALLOCATION_PID_OFFSET)) = getpid();
        *((int *)(bestFitAddress + ALLOCATION_LENGTH_OFFSET)) = size;
        *((int *)(bestFitAddress + ALLOCATION_PREV_OFFSET)) = bestFitAddress - bestFitPrevAddress;
        int remSizeInHole = *((int *)(bestFitAddress + ALLOCATION_LENGTH_OFFSET)) - size;
        // There is enough space left for a small hole, handle connection
        if (remSizeInHole > HEADER_SIZE)
        {
            // connect handle
            void *newHoleAddress = bestFitAddress + size + HEADER_SIZE;
            *((int *)(newHoleAddress)) = bestFitAddress + *((int *)(bestFitAddress)) - newHoleAddress;
            *((int *)(bestFitAddress)) = newHoleAddress - bestFitAddress;
            *((int *)(newHoleAddress + ALLOCATION_PREV_OFFSET)) = newHoleAddress - bestFitAddress;

            void *nextAddressPtr = newHoleAddress + *((int *)(newHoleAddress));
            *((int *)(nextAddressPtr + ALLOCATION_PREV_OFFSET)) = nextAddressPtr - newHoleAddress;

            //set settings of remaining hole
            *((char *)(newHoleAddress + ALLOCATION_USED_OFFSET)) = -1;
            *((int *)(newHoleAddress + ALLOCATION_LENGTH_OFFSET)) = remSizeInHole;
        }
        return bestFitAddress + HEADER_SIZE;
    }
    return NULL;
}

void *smem_worstFit(int size, void *shmPtr)
{
    void *worstFitAddress = NULL;
    void *worstFitPrevAddress = NULL;
    void *curMemPointer = shmPtr;
    void *prevAddress = NULL;
    int maxSizeFound = -1;
    while (*((int *)curMemPointer) > 0)
    {
        int sizeInBlock = *((int *)(curMemPointer + ALLOCATION_LENGTH_OFFSET));
        // current in between hole can be allocated to the requesting process
        if (*((char *)(curMemPointer + ALLOCATION_USED_OFFSET)) < 0 &&
            sizeInBlock >= size)
        {
            // check size
            if (maxSizeFound < sizeInBlock)
            {
                maxSizeFound = sizeInBlock;
                worstFitPrevAddress = prevAddress;
                worstFitAddress = curMemPointer;
            }
        }
        prevAddress = curMemPointer;
        curMemPointer += *((int *)(curMemPointer));
    }

    // check if there is enough memory at the end
    if (sharedMemorySize - (curMemPointer - shmPtr) > HEADER_SIZE + size)
    {
        // best fit could not be found, try to insert at the end
        if (worstFitAddress == NULL)
        {
            // allocate memory
            worstFitAddress = curMemPointer + HEADER_SIZE;
            *((char *)(curMemPointer + ALLOCATION_USED_OFFSET)) = 1;
            *((pid_t *)(curMemPointer + ALLOCATION_PID_OFFSET)) = getpid();
            *((int *)(curMemPointer + ALLOCATION_LENGTH_OFFSET)) = size;
            if (curMemPointer - shmPtr != 0)
            {
                *((int *)(curMemPointer + ALLOCATION_PREV_OFFSET)) = curMemPointer - prevAddress;
            }
            else
            {
                *((int *)(curMemPointer + ALLOCATION_PREV_OFFSET)) = -1;
            }

            // create new end tail
            void *tailHoleAddress = worstFitAddress + size;
            *((int *)(tailHoleAddress)) = -1;
            *((int *)(tailHoleAddress + ALLOCATION_PREV_OFFSET)) = tailHoleAddress - curMemPointer;
            *((int *)(curMemPointer)) = tailHoleAddress - curMemPointer;
            return worstFitAddress;
        }
        else // compare remaining memory and best fit
        {
            int remMemSize = sharedMemorySize - ((curMemPointer - shmPtr) + HEADER_SIZE);
            if (remMemSize > maxSizeFound && remMemSize >= size) // insert it end the tail
            {
                // allocate memory
                worstFitAddress = curMemPointer + HEADER_SIZE;
                *((char *)(curMemPointer + ALLOCATION_USED_OFFSET)) = 1;
                *((pid_t *)(curMemPointer + ALLOCATION_PID_OFFSET)) = getpid();
                *((int *)(curMemPointer + ALLOCATION_LENGTH_OFFSET)) = size;
                if (curMemPointer - shmPtr != 0)
                {
                    *((int *)(curMemPointer + ALLOCATION_PREV_OFFSET)) = curMemPointer - prevAddress;
                }
                else
                {
                    *((int *)(curMemPointer + ALLOCATION_PREV_OFFSET)) = -1;
                }

                // create new end tail
                void *tailHoleAddress = worstFitAddress + size;
                *((int *)(tailHoleAddress)) = -1;
                *((int *)(tailHoleAddress + ALLOCATION_PREV_OFFSET)) = tailHoleAddress - curMemPointer;
                *((int *)(curMemPointer)) = tailHoleAddress - curMemPointer;
            }
            else // insert it at the worstAddressFiund
            {
                *((char *)(worstFitAddress + ALLOCATION_USED_OFFSET)) = 1;
                *((pid_t *)(worstFitAddress + ALLOCATION_PID_OFFSET)) = getpid();
                *((int *)(worstFitAddress + ALLOCATION_LENGTH_OFFSET)) = size;
                *((int *)(worstFitAddress + ALLOCATION_PREV_OFFSET)) = worstFitAddress - worstFitPrevAddress;
                int remSizeInHole = *((int *)(worstFitAddress + ALLOCATION_LENGTH_OFFSET)) - size;
                // There is enough space left for a small hole, handle connection
                if (remSizeInHole > HEADER_SIZE)
                {
                    // connect handle
                    void *newHoleAddress = worstFitAddress + size + HEADER_SIZE;
                    *((int *)(newHoleAddress)) = worstFitAddress + *((int *)(worstFitAddress)) - newHoleAddress;
                    *((int *)(worstFitAddress)) = newHoleAddress - worstFitAddress;
                    *((int *)(newHoleAddress + ALLOCATION_PREV_OFFSET)) = newHoleAddress - worstFitAddress;

                    void *nextAddressPtr = newHoleAddress + *((int *)(newHoleAddress));
                    *((int *)(nextAddressPtr + ALLOCATION_PREV_OFFSET)) = nextAddressPtr - newHoleAddress;

                    //set settings of remaining hole
                    *((char *)(newHoleAddress + ALLOCATION_USED_OFFSET)) = -1;
                    *((int *)(newHoleAddress + ALLOCATION_LENGTH_OFFSET)) = remSizeInHole;
                }
                worstFitAddress = worstFitAddress + HEADER_SIZE;
            }
            return worstFitAddress;
        }
    }
    else if (worstFitAddress != NULL) // not enough memory left, chec worst address found to insert here
    {
        *((char *)(worstFitAddress + ALLOCATION_USED_OFFSET)) = 1;
        *((pid_t *)(worstFitAddress + ALLOCATION_PID_OFFSET)) = getpid();
        *((int *)(worstFitAddress + ALLOCATION_LENGTH_OFFSET)) = size;
        *((int *)(worstFitAddress + ALLOCATION_PREV_OFFSET)) = worstFitAddress - worstFitPrevAddress;
        int remSizeInHole = *((int *)(worstFitAddress + ALLOCATION_LENGTH_OFFSET)) - size;
        // There is enough space left for a small hole, handle connection
        if (remSizeInHole > HEADER_SIZE)
        {
            // connect handle
            void *newHoleAddress = worstFitAddress + size + HEADER_SIZE;
            *((int *)(newHoleAddress)) = worstFitAddress + *((int *)(worstFitAddress)) - newHoleAddress;
            *((int *)(worstFitAddress)) = newHoleAddress - worstFitAddress;
            *((int *)(newHoleAddress + ALLOCATION_PREV_OFFSET)) = newHoleAddress - worstFitAddress;

            void *nextAddressPtr = newHoleAddress + *((int *)(newHoleAddress));
            *((int *)(nextAddressPtr + ALLOCATION_PREV_OFFSET)) = nextAddressPtr - newHoleAddress;

            //set settings of remaining hole
            *((char *)(newHoleAddress + ALLOCATION_USED_OFFSET)) = -1;
            *((int *)(newHoleAddress + ALLOCATION_LENGTH_OFFSET)) = remSizeInHole;
        }
        return worstFitAddress + HEADER_SIZE;
    }
    return NULL;
}

// HEADER INFO
// 4 byte - next header offset (int)
// 4 byte - previous header offset (int)
// 4 byte - pid (pid_t)
// 1 byte - isUsed signed char (char)
// 4 byte - length of the allocated size for the hole (int)
// size bytes where size is denoted in the length info.