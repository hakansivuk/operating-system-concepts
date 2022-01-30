#include <stdlib.h>
#include <stdio.h>
#include <mqueue.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>

#include "shareddefs.h"
#include "ChildProcess.h"
#include "pwc.h"

mqd_t mqs[5];
struct mq_attr mqattrs[5];
struct item *itemPtrs[5];
int receivedBytes[5];
int bufLens[5];
char *bufPtrs[5];

int main(int argc, char *argv[])
{
  struct timeval start;
  struct timeval end;
  struct timeval startTime;
  struct timeval endTime;

  gettimeofday(&startTime, NULL);
  int childCount = atoi(argv[1]);
  if (childCount <= 0 || childCount > 5)
  {
    printf("number of arguments must be between 1 and 5 inclusive, closing...\n ");
    return -1;
  }

  for (int i = 0; i < childCount; i++)
  {
    if (configureMessageQueue(i) < 0)
    {
      return -1;
    }
  }

  pid_t id;
  gettimeofday(&start, NULL);
  for (int i = 0; i < childCount; i++)
  {
    id = fork();
    if (id < 0)
    {
      printf("Error when creating child!\n");
    }
    else if (id == 0)
    {
      readFile(argv[i + 2], i);
      return 0;
      printf("You must not seee this, if you see this, serious bug in child process!");
    }
    else
    {
      printf("Child with id: %d is created.\n", i);
    }
  }
  gettimeofday(&end, NULL);

  printf("Process creation time:\n");
  printf("\tIn seconds: %d\n", end.tv_sec - start.tv_sec);
  printf("\tIn microseconds: %ld\n",
         (end.tv_sec * 1000000 + end.tv_usec) -
             (start.tv_sec * 1000000 + start.tv_usec));

  if (checkMessages(childCount) < 0)
  {
    return -1;
  }
  gettimeofday(&endTime, NULL);

  printf("Program run time:\n");
  printf("\tIn seconds: %d\n", endTime.tv_sec - startTime.tv_sec);
  printf("\tIn microseconds: %ld\n",
         (endTime.tv_sec * 1000000 + endTime.tv_usec) -
             (startTime.tv_sec * 1000000 + startTime.tv_usec));

  return 0;
}

int configureMessageQueue(int id)
{
  char name[32];
  getFileName(name, id);
  mqs[id] = mq_open(name, O_CREAT | O_RDWR, 0666, NULL);
  mq_getattr(mqs[id], &mqattrs[id]);
  mqattrs[id].mq_maxmsg = 1000000;
  mq_setattr(mqs[id], &mqattrs[id], NULL);

  bufLens[id] = mqattrs[id].mq_msgsize;
  bufPtrs[id] = (char *)malloc(bufLens[id]);
  if (mqs[id] == -1)
  {
    printf("Memeory queue opneining on host has failed!\n");
    return -1;
  }
  return 1;
}

int checkMessages(int childCount)
{
  struct timeval start;
  struct timeval end;

  // Initial data, fist smallest words, waits for all message queues (childs to be ready)
  gettimeofday(&start, NULL);
  for (int i = 0; i < childCount; i++)
  {
    printf("Waiting child: %d\n", i);
    receivedBytes[i] = mq_receive(mqs[i], bufPtrs[i], bufLens[i], NULL);
    if (receivedBytes[i] == -1)
    {
      printf("Error no message received on child: %d!\n", i);
      return -1;
    }
    itemPtrs[i] = (struct item *)bufPtrs[i];
  }
  gettimeofday(&end, NULL);

  printf("Processes finished processing in time:\n");
  printf("\tIn seconds: %d\n", end.tv_sec - start.tv_sec);
  printf("\tIn microseconds: %ld\n",
         (end.tv_sec * 1000000 + end.tv_usec) -
             (start.tv_sec * 1000000 + start.tv_usec));

  // algorithm to write to an output file:
  // find shortest word with duplicates if any, sum the counts, write it to a file,
  // receive another message from this queue that has the smallest message if there is any, repeat
  struct timespec dropTime;
  clock_gettime(CLOCK_REALTIME, &dropTime);
  dropTime.tv_sec += 3;
  int available[childCount];
  int hasBeenRead[childCount];
  for (int i = 0; i < childCount; i++)
  {
    available[i] = 1;
    hasBeenRead[i] = 0;
  }

  FILE *outputFile = fopen("output_pwc.txt", "w+");
  if (outputFile == NULL)
  {
    printf("FILE cannot be created for output...\n");
  }

  int remainingChildCount = childCount;
  int smallestFound;
  char smallestWord[1024];
  char *smallest = malloc(1024 * sizeof(char));
  int count;
  gettimeofday(&start, NULL);
  while (remainingChildCount > 0)
  {
    // find smallest word in available buffers
    smallestFound = -1;
    for (int i = 0; i < childCount; i++)
    {
      if (available[i])
      {
        if (smallestFound < 0)
        {
          strcpy(smallest, (const char *)itemPtrs[i]->astr);
          smallestFound = 1;
        }
        else
        {
          if (strcmp((const char *)smallest, (const char *)itemPtrs[i]->astr) > 0)
          {
            strcpy(smallest, (const char *)itemPtrs[i]->astr);
          }
        }
      }
    }

    // find duplicates if any and add to the smallest word
    count = 0;
    for (int i = 0; i < childCount; i++)
    {
      if (available[i])
      {
        if (strcmp((const char *)smallest, (const char *)itemPtrs[i]->astr) == 0)
        {
          hasBeenRead[i] = 1;
          count = count + itemPtrs[i]->count;
        }
      }
    }

    // write to file
    if (outputFile != NULL)
    {
    	fprintf( outputFile, "%s %d\n", smallest, count);
    }

    // read from the buffers that are used for output
    for (int i = 0; i < childCount; i++)
    {
      if (available[i] && hasBeenRead[i] > 0)
      {
        receivedBytes[i] = mq_timedreceive(mqs[i], bufPtrs[i], bufLens[i], NULL, &dropTime);
        hasBeenRead[i] = 0;
        if (receivedBytes[i] < 0)
        {
          printf("Error on reading child: %d, queue is empty!\n", i);
          available[i] = 0;
          remainingChildCount--;
        }
        itemPtrs[i] = (struct item *)bufPtrs[i];
      }
    }
  }
  gettimeofday(&end, NULL);

  printf("Process merge algorithm end time:\n");
  printf("\tIn seconds: %d\n", end.tv_sec - start.tv_sec);
  printf("\tIn microseconds: %ld\n",
         (end.tv_sec * 1000000 + end.tv_usec) -
             (start.tv_sec * 1000000 + start.tv_usec));

  fclose(outputFile);
  free(smallest);
  printf("All message queus are empty. Output file generated. Parent process has finished...\n");
  for (int i = 0; i < childCount; i++)
  {
    free(bufPtrs[i]);
    mq_close(mqs[i]);

    char name[32];
    getFileName(name, i);
    mq_unlink(name);
  }
  return 0;
}