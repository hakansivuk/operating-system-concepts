# Multiprocessing vs Multithreading

This is a simple C project that compares the performance of multiprocessing and multithreading while reading multiple text files concurrently to produce an output of word frequency. Each process/thread reads a file and generates a linked-list of words sorted by their frequencies. Each node in the list contains the word, its frequency and a pointer to the next node.

In a multiprocessing environment, message queues are used to transfer information to the parent process. The parent process creates a message queue for each child process and each child sends their data node by node. Thus, the parent process receives sorted words so as to write them to an output file, sorting every word among text files by their frequencies.

In a multithreading environment, the main thread waits for all threads to finish sorting their words, then the main thread accomplished the task of sorting sorted words while writing to an output file.

This project allowed me to understand the differences between multiprocessing and multithreading while developing my first C project. 

## How to Run

> Compile the application using the Make file.
> Execute by:
> ./pwc N f1 f2 ... fN outfile (for multiprocessing)
> ./twc N  f1 f2 ... fN outfile (for multithreading)

*	N is the number of files to process.
*	f1, f2, ... fN are file names.
*	outfile is the name of the output file to write words sorted by their frequencies.

For further technical details, you could refer to _project1.pdf_ file.
