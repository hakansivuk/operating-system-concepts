#include <stdio.h>
#include <sys/time.h>
#include <string.h>
#include <stdlib.h>
#include "list.h"

// struct LinkedList *createLinkedList();
// void destroyLinkedList( struct LinkedList *list);
// void addNode( struct LinkedList *list, char *data);
// void traverseList( struct LinkedList *list);
// void sortByInsertion(struct LinkedList *list );
// //int compareNodes( char word[], struct Node *to);
// //void swapNodes( struct Node *from, struct Node *to);
// int getRandomNumber( int lowerBound, int upperBound);
// struct Node *getHead( struct LinkedList *list);

// struct LinkedList
// {
// 	struct Node *head;
// 	struct Node *tail;
// };

// struct Node
// {
// 	struct Node *next;
// 	char word[1024];
// 	int count;
// };

//int NUM_COUNT = 10000;

// int main( int argc. char *argv[])
// {
// 	// This is my first C program!
// 	printf( "Hello World!\n");
// 	struct LinkedList *myList = createLinkedList();

// 	// Set random seed
// 	srand( time( 0));

// 	struct timeval startTime;
// 	struct timeval finishTime;

// 	gettimeofday( &startTime, NULL);
// 	for ( int i = 0; i < NUM_COUNT; i++)
// 	{
// 		int ranNum = getRandomNumber( 0, NUM_COUNT);
// 		addNode( myList, ranNum);
// 	}
// 	gettimeofday( &finishTime, NULL);

// 	printf( "Time of execution to add 10,000 numbers:\n");
// 	printf( "\tIn seconds: %d\n", finishTime.tv_sec - startTime.tv_sec);
// 	printf( "\tIn microseconds: %ld\n",
// 		(finishTime.tv_sec * 1000000 + finishTime.tv_usec) -
// 		 (startTime.tv_sec * 1000000 + startTime.tv_usec) );

// 	destroyLinkedList( myList);

// 	return 0;
// }

struct LinkedList *createLinkedList()
{
	struct LinkedList *list = malloc(sizeof(struct LinkedList));
	list->head = NULL;
	list->tail = NULL;

	return list;
}

struct Node *getHead(struct LinkedList *list)
{
	return list->head;
}

void destroyLinkedList(struct LinkedList *list)
{
	// free memory
	struct Node *prev;
	while (list->head != NULL)
	{
		prev = list->head;
		list->head = list->head->next;

		//free( prev->word);
		free(prev);
	}
	list->head = NULL;
	list->tail = NULL;
	prev = NULL;
	free(list);
}

void addNode(struct LinkedList *list, char *data)
{
	if (list->head == NULL)
	{
		list->head = malloc(sizeof(struct Node));
		list->tail = list->head;

		strcpy(list->head->word, data);
		list->head->count = 1;
		list->head->next = NULL;
	}
	else
	{
		list->tail->next = malloc(sizeof(struct Node));
		list->tail = list->tail->next;

		list->tail->next = NULL;
		strcpy(list->tail->word, data); // list->tail->word = data;
		list->tail->count = 1;
	}
}

struct Node *hasData(struct LinkedList *list, char *word)
{
	struct Node *cur = list->head;
	while (cur != NULL)
	{
		if (strcmp(cur->word, word) == 0)
		{
			return cur;
		}
		cur = cur->next;
	}
	return NULL;
}

void traverseList(struct LinkedList *list)
{
	struct Node *cur = list->head;
	while (cur != NULL)
	{
		printf("Data is: %s\t\t\tNumber is: %d\n", cur->word, cur->count);
		cur = cur->next;
	}
}

void swapNodes(struct Node *from, struct Node *to)
{
	char tempWord[1024];
	strcpy(tempWord, from->word);
	int tempCount = from->count;

	strcpy(from->word, to->word);
	from->count = to->count;

	strcpy(to->word, tempWord);
	to->count = tempCount;
}

int compareNodes(char word[], struct Node *to)
{
	return strcmp(word, to->word);
}

void sortByInsertion(struct LinkedList *list)
{
	struct Node *cur;
	struct Node *smallest;
	char smallWord[1024];
	for (struct Node *i = list->head; i != NULL; i = i->next)
	{
		smallest = NULL;
		strcpy(smallWord, i->word);
		cur = i->next;
		while (cur != NULL)
		{
			if (compareNodes(smallWord, cur) > 0)
			{
				smallest = cur;
				strcpy(smallWord, cur->word);
			}
			cur = cur->next;
		}

		if (smallest != NULL)
		{
			swapNodes(i, smallest);
		}
	}
}