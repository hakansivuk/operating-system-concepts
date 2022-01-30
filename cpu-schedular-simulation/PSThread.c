#include "PSThread.h"
#include "CommonFuncs.h"
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include "lists/list.h"

extern char *fileName;
extern struct programData data;
extern struct threadargs threadParams[NUM_OF_THREADS];
extern int done[NUM_OF_THREADS];
extern struct LinkedList *readyQueue;
extern pthread_cond_t waitPacket;

void *runner(void *param)
{
    int threadId = ((struct threadargs *)param)->id;
    pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
    printf("Process %d begins\n", threadId + 1);

    FILE *infile = NULL;
    if (fileName != NULL)
    {
        printf("Filename check for thread\n");
        char modifiedFileName[128];
        getFileName(modifiedFileName, threadId);
        printf("Thread with id: %d is opening arg file: %s\n", threadId, modifiedFileName);

        infile = fopen(modifiedFileName, "r");
    }

    if (infile != NULL)
    {
        while (!feof(infile))
        {
            // Get Burst Duration
            int burstTime = getCPUBurstDuration(infile);
            // Send burst duration to queue
            addNode(readyQueue, threadId, burstTime, 1);
            pthread_cond_signal(&waitPacket);
            // wait for mutex conditional variable
            pthread_cond_wait(&threadParams[threadId].cond, &lock);
            // Get Sleep Duration
            int sleepTime = getSleepDuration(infile);
            usleep(sleepTime * 1000); // Sleep in ms!!!!!
        }
        fclose(infile);
    }
    else
    {
        int i = 0;
        while (i < data.duration)
        {
            // Get Burst Duration
            int burstTime = getCPUBurstDuration(infile);
            // Send burst duration to queue
            addNode(readyQueue, threadId, burstTime, 1);
            pthread_cond_signal(&waitPacket);
            // wait for mutex conditional variable
            pthread_cond_wait(&(threadParams[threadId].cond), &lock);
            // Get Sleep Duration
            int sleepTime = getSleepDuration(infile);
            usleep(sleepTime * 1000); // Sleep in ms!!!!!
            i++;
        }
    }
    addNode(readyQueue, threadId, -1, 0);
    pthread_cond_signal(&waitPacket);
    pthread_exit(0);
}

int getCPUBurstDuration(FILE *fp)
{
    int burstTime = 0;
    if (fp == NULL)
    {
        burstTime = getRandomNum(data.minCPU, data.maxCPU, 100);
    }
    else
    {
        // READ burst time from file and assign it to burstTime
        fscanf(fp, "%*s %d", &burstTime);
    }
    return burstTime;
}

int getSleepDuration(FILE *fp)
{
    int sleepTime = 0;
    if (fp == NULL)
    {
        sleepTime = getRandomNum(data.minIO, data.maxIO, 100);
    }
    else
    {
        // READ burst time from file and assign it to burstTime
        fscanf(fp, "%*s %d", &sleepTime);
    }
    return sleepTime;
}