void *runner(void *param);

#define NUM_OF_THREADS 10

struct threadargs
{
    int id;
    char fileName[128];
} threadargs;