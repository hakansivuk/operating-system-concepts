#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>

#include "twc.h"
#include "lists/list.h"
#include "lists/WordList.h"

pthread_t tids[NUM_OF_THREADS];
pthread_attr_t attrs[NUM_OF_THREADS];
struct WordLinkedList *wordLists[NUM_OF_THREADS];
struct threadargs threadParams[NUM_OF_THREADS];
int main(int argc, char *argv[])
{
    struct timeval start;
    struct timeval end;
    struct timeval startTime;
    struct timeval endTime;

    gettimeofday(&startTime, NULL);
    int threadCount = atoi(argv[1]);
    if (threadCount <= 0)
    {
        printf("Number of argumnets must be between 1 and 5 inclusive...");
        return -1;
    }

    // Cretate threads
    gettimeofday(&start, NULL);
    for (int i = 0; i < threadCount; i++)
    {
        wordLists[i] = createWordList();
        pthread_attr_init(&attrs[i]);

        threadParams[i].id = i;
        strcpy(threadParams[i].fileName, argv[i + 2]);

        pthread_create(&tids[i], &attrs[i], runner, &threadParams[i]);
        printf("Thread created with id: %d\n", i);
    }
    gettimeofday(&end, NULL);

    printf("Thread creation time:\n");
    printf("\tIn seconds: %d\n", end.tv_sec - start.tv_sec);
    printf("\tIn microseconds: %ld\n",
           (end.tv_sec * 1000000 + end.tv_usec) -
               (start.tv_sec * 1000000 + start.tv_usec));

    gettimeofday(&start, NULL);
    printf("Waiting for threads...\n");
    for (int i = 0; i < threadCount; i++)
    {
        pthread_join(tids[i], NULL);
    }
    gettimeofday(&end, NULL);
    printf("Threads completed...\n");

    printf("Threads completed their tasks in time:\n");
    printf("\tIn seconds: %d\n", end.tv_sec - start.tv_sec);
    printf("\tIn microseconds: %ld\n",
           (end.tv_sec * 1000000 + end.tv_usec) -
               (start.tv_sec * 1000000 + start.tv_usec));

    FILE *fp = fopen("output_twc.txt", "w+");

    // Merge solutions
    gettimeofday(&start, NULL);
    struct Node *heads[NUM_OF_THREADS];
    int available[NUM_OF_THREADS];
    int hasBeenRead[NUM_OF_THREADS];
    for (int i = 0; i < threadCount; i++)
    {
        heads[i] = getHeadNode(wordLists[i]);
        available[i] = 1;
        hasBeenRead[i] = 0;
    }

    int remianingLists = threadCount;
    int smallestFound;
    char *smallest = malloc(1024 * sizeof(char));
    int count;
    while (remianingLists > 0)
    {
        // find smallest word in available buffers
        smallestFound = -1;
        for (int i = 0; i < threadCount; i++)
        {
            if (available[i])
            {
                if (smallestFound < 0)
                {
                    strcpy(smallest, (const char *)heads[i]->word);
                    smallestFound = 1;
                }
                else
                {
                    if (strcmp((const char *)smallest, (const char *)heads[i]->word) > 0)
                    {
                        strcpy(smallest, (const char *)heads[i]->word);
                    }
                }
            }
        }

        count = 0;
        for (int i = 0; i < threadCount; i++)
        {
            if (available[i])
            {
                if (strcmp((const char *)smallest, (const char *)heads[i]->word) == 0)
                {
                    hasBeenRead[i] = 1;
                    count = count + heads[i]->count;
                }
            }
        }

        // write to file
        if (fp != NULL)
        {
            fprintf( fp, "%s %d\n", smallest, count);
        }

        // read from the buffers that are used for output
        for (int i = 0; i < threadCount; i++)
        {
            if (available[i] && hasBeenRead[i] > 0)
            {
                heads[i] = heads[i]->next;
                hasBeenRead[i] = 0;
                if (heads[i] == NULL)
                {
                    available[i] = 0;
                    remianingLists--;
                }
            }
        }
    }
    gettimeofday(&end, NULL);

    printf("Algorithm completed for threads:\n");
    printf("\tIn seconds: %d\n", end.tv_sec - start.tv_sec);
    printf("\tIn microseconds: %ld\n",
           (end.tv_sec * 1000000 + end.tv_usec) -
               (start.tv_sec * 1000000 + start.tv_usec));

    // Clear up
    fclose(fp);
    free(smallest);
    for (int i = 0; i < threadCount; i++)
    {
        destroyWordLinkedList(wordLists[i]);
    }
    gettimeofday(&endTime, NULL);

    printf("Program run for:\n");
    printf("\tIn seconds: %d\n", endTime.tv_sec - startTime.tv_sec);
    printf("\tIn microseconds: %ld\n",
           (endTime.tv_sec * 1000000 + endTime.tv_usec) -
               (startTime.tv_sec * 1000000 + startTime.tv_usec));
    return 0;
}

void *runner(void *param)
{
    // Open and generate word list

    int threadID = ((struct threadargs *)param)->id;
    FILE *fp = fopen(((struct threadargs *)param)->fileName, "r");
    char word[1024];
    while (fscanf(fp, "%s", word) != EOF)
    {
        addWord(wordLists[threadID], word);
    }
    fclose(fp);
    sort(wordLists[threadID]);

    pthread_exit(0);
}