#include <stdlib.h>
#include <stdio.h>
#include <mqueue.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include "shareddefs.h"
#include "ChildProcess.h"
#include "lists/WordList.h"
#include "lists/list.h"

mqd_t mq;
struct item myItem;
int n;

int readFile(char fileName[], int childID)
{
    struct WordLinkedList *myList = createWordList();

    // Open and generate word list
    FILE *fp = fopen(fileName, "r");
    char word[1024];
    while (fscanf(fp, "%s", word) != EOF)
    {
        addWord(myList, word);
    }
    fclose(fp);
    sort(myList);

    char name[32];
    getFileName(name, childID);
    mq = mq_open(name, O_RDWR);
    if (mq == -1)
    {
        printf("Child could not open shared memory!\n");
        return -1;
    }

    struct Node *cur = getHeadNode(myList);
    int i = 0;
    while (cur != NULL)
    {
        myItem.id = i;
        myItem.count = cur->count;
        strcpy(myItem.astr, cur->word);
        n = mq_send(mq, (char *)&myItem, sizeof(struct item), 0);
        if (n == -1)
        {
            printf("Child with id: %d could not send message...\n", childID);
            return -1;
        }

        cur = cur->next;
        i++;
    }
    mq_close(mq);
    destroyWordLinkedList(myList);
    return 0;
}