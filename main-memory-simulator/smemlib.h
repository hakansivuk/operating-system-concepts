#ifndef SMEMLIB_H
#define SMEMLIB_H

void *smem_firstFit(int size, void *shmPtr);
void *smem_bestFit(int size, void *shmPtr);
void *smem_worstFit(int size, void *shmPtr);

int smem_init(int segmentsize);
int smem_remove();

int smem_open();
void *smem_alloc(int size);
void smem_free(void *p);
int smem_close();
void smem_library_free(void *p);
int smem_get_mem_utilization(int *totUnusedSize);
#endif
