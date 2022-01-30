#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "smemlib.h"

struct ExperimentResult{
    float failRate;
    float averageUnusedBlockCount;
    float averageUnusedSizeLength;
};

int getRandomInt(int i)
{
    srand(i);
    int j = rand() % 12;
    return j;
}

void startExperiment(int pid){
    int failCount = 0;
    int unusedBlockCount = 0;
    int unusedSizeLength = 0;
    int totalUnusedBlockCount = 0;
    int totalUnusedSizeLength = 0;

    int libStatus = smem_open();
    if (libStatus >= 0)
    {
        int *pointers[12] = {NULL};
        int count[12] = {0};

        for (int i = 0; i < 1000; i++)
        {

            int index = getRandomInt(i);
            count[index]++;
            if (pointers[index] == NULL)
            {
                pointers[index] = smem_alloc((index + 1) * 2048);
                if (pointers[index] == NULL)
                    failCount++;
            }
            else
            {
                smem_free(pointers[index]);
                pointers[index] = NULL;

                unusedBlockCount = smem_get_mem_utilization(&unusedSizeLength);
                totalUnusedBlockCount += unusedBlockCount;
                totalUnusedSizeLength += unusedSizeLength;
            }
        }
    }
    printf("");
    smem_close();
    printf("(%d)\tAverage unused block count is: %f\n", pid, totalUnusedBlockCount/1000.0);
    printf("(%d)\tAverage unused size is %f bytes.\n", pid, totalUnusedSizeLength/1000.0);
    printf("(%d)\tFail rate is %f\n", pid, failCount/1000.0);
}

int main()
{
    pid_t n;
    smem_init(32768);

    for(int i = 0 ; i < 10 ; i++){
        n = fork();
        if(n == 0){ // child
            startExperiment(i);
            exit(0);
        }
    }

    for(int i = 0 ; i < 10 ; i++){
        wait(NULL);
    }
    
    return 0;
}
