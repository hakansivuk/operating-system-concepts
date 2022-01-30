#include "list.h"
#include "WordList.h"
#include <stdio.h>
#include <stdlib.h>

// struct WordLinkedList *createWordList();
// void destroyWordLinkedList( struct WordLinkedList *wordList);
// void addWord( struct WordLinkedList *wordList, char* word);
// struct Node *getHeadNode( struct WordLinkedList *wordList);
// void sort();

// struct WordLinkedList
// {
//     struct LinkedList* list;
// };

struct WordLinkedList *createWordList()
{
    struct WordLinkedList *wordList = malloc(sizeof(struct WordLinkedList));
    wordList->list = createLinkedList();
    return wordList;
}

void destroyWordLinkedList(struct WordLinkedList *wordList)
{
    destroyLinkedList(wordList->list);
    wordList->list = NULL;

    free(wordList);
    wordList = NULL;
}

void addWord(struct WordLinkedList *wordList, char *word)
{
    struct Node *wordNode = hasData(wordList->list, word);
    if (wordNode == NULL)
    {
        addNode(wordList->list, word);
    }
    else
    {
        wordNode->count++;
    }
}

struct Node *getHeadNode(struct WordLinkedList *wordList)
{
    return getHead(wordList->list);
}

void sort(struct WordLinkedList *wordList)
{
    sortByInsertion(wordList->list);
}