#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include "schedule.h"
#include "PSThread.h"
#include "CommonFuncs.h"
#include <unistd.h>
#include "lists/list.h"

char *fileName; // Change
struct programData data;
struct threadargs threadParams[NUM_OF_THREADS];
struct LinkedList *readyQueue;
int done[NUM_OF_THREADS];
pthread_cond_t waitPacket = PTHREAD_COND_INITIALIZER;
pthread_mutex_t test = PTHREAD_MUTEX_INITIALIZER;

int totWaitingTime[NUM_OF_THREADS];
int totResponseTime[NUM_OF_THREADS];
int responseCount[NUM_OF_THREADS];

int main(int argc, char *argv[])
{
    init();
    int threadCount = atoi(argv[1]);
    data.minCPU = atoi(argv[2]);
    data.maxCPU = atoi(argv[3]);
    data.minIO = atoi(argv[4]);
    data.maxIO = atoi(argv[5]);
    char *outputName = argv[6];
    data.duration = atoi(argv[7]);
    char *algorithm = argv[8];
    data.quantum = atoi(argv[9]);
    if (strcmp(argv[10], "no-infile") == 0)
    {
        data.infile = NULL;
        fileName = NULL;
    }
    else
    {
        data.infile = argv[10];
        fileName = argv[10];
    }

    printf("\nThread count:%d, Mincpu:%d, MaxCpu:%d, MinIO:%d, maxIO: %d\n", threadCount, data.minCPU, data.maxCPU, data.minIO, data.maxIO);
    printf("Outputname: %s, duration:%d, alg:%s, quantum:%d, infile:%s\n\n", outputName, data.duration, algorithm, data.quantum, data.infile);

    if (threadCount <= 0)
    {
        printf("Number of argumnets must be between 1 and 5 inclusive...\n");
        return -1;
    }

    // Select algorithm
    struct BurstNode *(*getNextNode)(struct LinkedList *, int *);
    if (strcmp(algorithm, "FCFS") == 0)
        getNextNode = &FCFS;
    else if (strcmp(algorithm, "SJF") == 0)
        getNextNode = &SJF;
    else
        getNextNode = &RR;

    //Create Queue here! with respect to the algorithm!
    readyQueue = createLinkedList();

    for (int i = 0; i < threadCount; i++)
    {
        totWaitingTime[i] = 0;
        totResponseTime[i] = 0;
        responseCount[i] = 0;
    }

    // Cretate threads
    for (int i = 0; i < threadCount; i++)
    {
        pthread_attr_init(&threadParams[i].attr);

        threadParams[i].id = i;
        threadParams[i].cond = (pthread_cond_t)PTHREAD_COND_INITIALIZER;

        pthread_create(&threadParams[i].tid, &threadParams[i].attr, runner, &threadParams[i]);
        printf("Thread created with id: %d\n", i);
    }
    printf("All threads are created\n");

    // Open output file to write!
    FILE *fp = fopen(outputName, "w+");
    if (fp == NULL)
    {
        printf("Output file could not be opened!\n");
        return -1;
    }
    // Write here the SE thread logic
    printf("output file is opened\n");
    int flag = 0; // depreciated
    int sum = threadCount;
    struct timeval startTime;
    struct timeval exeTime;
    int timeFlag = 1;
    while (sum > 0)
    {
        flag = 0;                                                    // depreciated
        struct BurstNode *curBurst = getNextNode(readyQueue, &flag); // flag arg is depreciated, is not used, can be remvoed to increase efficiency
        if (curBurst == NULL)
        {
            pthread_mutex_lock(&test);
            pthread_cond_wait(&waitPacket, &test);
            pthread_mutex_unlock(&test);
            curBurst = getNextNode(readyQueue, &flag);
        }

        if (curBurst->burstTime <= 0)
        {
            sum--;
            free(curBurst);
            continue;
        }

        if (timeFlag > 0)
        {
            gettimeofday(&startTime, NULL);
            exeTime = startTime;
            timeFlag = 0;
        }
        else
        {
            gettimeofday(&exeTime, NULL);
        }

        int waitingTime = ((exeTime.tv_sec - curBurst->entryTime.tv_sec) * 1000000 + (exeTime.tv_usec - curBurst->entryTime.tv_usec)) / 1000;
        totWaitingTime[curBurst->id] += waitingTime;

        if (curBurst->first == 1)
        {
            struct timeval entryTime = curBurst->entryTime;
            totResponseTime[curBurst->id] += ((exeTime.tv_sec - entryTime.tv_sec) * 1000000 + (exeTime.tv_usec - entryTime.tv_usec)) / 1000;
            responseCount[curBurst->id]++;
        }

        if(curBurst->last == 1) usleep(curBurst->burstTime * 1000);
        else usleep(data.quantum * 1000);
        //writeOutput(fp, (exeTime.tv_sec - startTime.tv_sec) * 1000000 + (exeTime.tv_usec - startTime.tv_usec), curBurst->burstTime, curBurst->id);

        if (curBurst->last == 1){ //if (flag == 0)
            pthread_cond_signal(&(threadParams[curBurst->id].cond));
            writeOutput(fp, (exeTime.tv_sec - startTime.tv_sec) * 1000000 + (exeTime.tv_usec - startTime.tv_usec), curBurst->burstTime, curBurst->id);
        }
        else{
            addNode(readyQueue, curBurst->id, curBurst->burstTime - data.quantum, 0);
            writeOutput(fp, (exeTime.tv_sec - startTime.tv_sec) * 1000000 + (exeTime.tv_usec - startTime.tv_usec), data.quantum, curBurst->id);
        }
        free(curBurst);
    }

    for (int i = 0; i < threadCount; i++)
    {
        pthread_join(threadParams[i].tid, NULL);
        if (responseCount[i] > 0)
        {
            printf("Statistics for process: %d\n", i + 1);
            printf("\tTotal waiting time: %d ms.\n", totWaitingTime[i]);
            printf("\tAverage response time: %f ms.\n", totResponseTime[i] * 1.0 / responseCount[i]);
        }
    }

    free(readyQueue);
    fclose(fp);
    return 0;
}