#include <stdio.h>
#include <sys/time.h>
#include <string.h>
#include <stdlib.h>

struct LinkedList *createLinkedList();
void destroyLinkedList(struct LinkedList *list);
void addNode(struct LinkedList *list, char *data);
void traverseList(struct LinkedList *list);
void sortByInsertion(struct LinkedList *list);
//int compareNodes(char word[], struct Node *to);
//void swapNodes(struct Node *from, struct Node *to);
struct Node *getHead(struct LinkedList *list);
struct Node *hasData(struct LinkedList *list, char *word);

struct LinkedList
{
	struct Node *head;
	struct Node *tail;
};

struct Node
{
	struct Node *next;
	char word[1024];
	int count;
};