#ifndef LIST_H
#define LIST_H

#include <stdio.h>
#include <sys/time.h>
#include <string.h>
#include <stdlib.h>

struct LinkedList *createLinkedList();
void addNode(struct LinkedList *list, int id, int burstTime, int first);
struct BurstNode* FCFS(struct LinkedList* list, int* flag);
struct BurstNode* SJF(struct LinkedList* list, int* flag);
struct BurstNode* RR(struct LinkedList* list, int* flag);

struct LinkedList
{
	struct BurstNode* head;
	struct BurstNode* tail;
};

struct BurstNode
{
	struct BurstNode *next;
	int id;
	int burstTime;
	int initialBurstTime;
	int first;
	int last;
	struct timeval entryTime;
};

#endif