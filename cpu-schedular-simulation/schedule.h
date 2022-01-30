#ifndef SCHEDULE_H
#define SCHEDULE_H

#include <stdio.h>
#define NUM_OF_THREADS 5

struct programData
{
    int minCPU;
    int maxCPU;
    int minIO;
    int maxIO;
    int duration;
    char *infile;
    int quantum;
};

#endif