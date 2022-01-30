

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

#include "smemlib.h"

int main()
{

    int res = smem_init(32768);
    printf("res is %d\n", res);

    printf("memory segment is ready to use.\n");

    return (0);
}
