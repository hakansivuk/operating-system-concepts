# Main Memory Simulator

Main Memory simulator is a C application that provides a memory management library for processes. This library provides dynamic memory allocation and deallocation for processes that wish to use main memory. The supported memory fit strategies are first-fit, best-fit and worst-fit togethet with hole-list approach. 

## How to Use

> Compile your application with smemlib.a. Import "smemlib.h" to use the library. Here is the basic documentation:

* **int smem_init (int segsize):** This function creates and initializes a shared memory segment of the given size. The given size is in bytes and must be a power of 2. Memory is allocated from that segment to the requesting processes. If operation is successful, the function will return 0, otherwise, it will return -1.
* **smem_remove ():** This function removes the shared memory segment from the system.
* **int smem_open ():** This function indicates to the library that the process would like to use the library. If there are too many processes using the library at the moment, sem_open will return -1. Otherwise, if the process can use the library, sem_open will return 0.
* **void \*smem_alloc (int reqsize):** This function allocates memory of size at least the requested size (reqsize) and returns a pointer to the allocated space. It is up to the program what to store in the allocated space. NULL will be returned if memory could not be allocated. This can happen, for example, when there is not enough memory.
* **void smem_free (void \*ptr):** This function will deallocate the memory space allocated earlier and pointed by the pointer ptr. The deallocated memory space will be part of the free memory in the segment.
* **int smem_close ():** When a process has finished using the library it should call smem_close(), hence shared segment can be unmapped from the virtual address space of the process. If the process would like to use the library again, it should call smem_open again.

## Example

	#include “smemlib.h”
    
    #DEFINE ASIZE 1024 
    int main()
	 { 
		int i, ret;
		char *p;

		ret = smem_open(); 
		if (ret == -1) 
		{
			exit( 1); // can not use the library at the moment. May try later. 
		}

		p = smem_alloc( ASIZE); // allocate space to hold 1024 characters 
		for (i = 0; i < ASIZE; i++)
		{
			p[ i] = ‘a’; // initialize all chars to ‘a’
		}
		smem_free( p);
		smem_close(); // no longer interested in using the library.
		return (0); 
	}
