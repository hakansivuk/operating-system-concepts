all: process thread

process: pwc.c lists/list.c lists/WordList.c ChildProcess.c shareddefs.c
	gcc -Wall -o pwc pwc.c lists/list.c lists/WordList.c ChildProcess.c shareddefs.c -lrt
thread: twc.c lists/WordList.c lists/list.c 
	gcc -Wall -o twc twc.c lists/WordList.c lists/list.c -lpthread
clean:
	rm pwc twc