#include <pthread.h>
#include "schedule.h"
#include <stdio.h>

void *runner(void *param);
int getCPUBurstDuration(FILE *fp);
int getSleepDuration(FILE *fp);

struct threadargs
{
    int id;
    pthread_t tid;
    pthread_attr_t attr;
    pthread_cond_t cond;
};