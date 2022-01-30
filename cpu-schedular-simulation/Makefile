all: thread

thread: *.c lists/*.c
	gcc -Wall -o schedule *.c lists/*.c -lpthread
clean:
	rm schedule