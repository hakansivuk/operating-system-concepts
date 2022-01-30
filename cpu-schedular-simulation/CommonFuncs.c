#include "CommonFuncs.h"
#include <stdio.h>
#include <stdlib.h>
#include "string.h"
#include <time.h>

void init()
{
    srand(time(NULL));
}

int getRandomNum(int min, int max, int interval) // 200, 300
{
    int randomNum = (rand() % (max - min + 100)) + min; // [200, 400)
    randomNum = ((randomNum) / 100) * 100;              // 250 => 300, < 200

    if (randomNum == min || randomNum == max)
    {
    }
    else if (randomNum < min)
    {
        printf("FAILURE min %d\n", randomNum);
        exit(0);
    }
    else if (randomNum > max)
    {
        printf("failure max %d\n", randomNum);
        exit(0);
    }
    else
    {
    }

    return randomNum;
}

void getFileName(char name[], int id)
{
    strcpy(name, fileName);
    int nameLength = strlen(name);
    name[nameLength] = '0' + (id + 1);
    name[nameLength + 1] = '.';
    name[nameLength + 2] = 't';
    name[nameLength + 3] = 'x';
    name[nameLength + 4] = 't';
    name[nameLength + 5] = '\0';
}

void writeOutput(FILE *fp, long int totExecTime, int burstTime, int threadId)
{
    char formattedTime[11];
    char strExecTime[11];
    sprintf(strExecTime, "%ld", totExecTime);
    for (int i = 0; i < 10; i++)
    {
        if (i < 10 - strlen(strExecTime))
            formattedTime[i] = '0';
        else
            formattedTime[i] = strExecTime[i - 10 + strlen(strExecTime)];
    }
    formattedTime[10] = '\0';
    fprintf(fp, "%s %d %d\n", formattedTime, burstTime, threadId + 1);
}