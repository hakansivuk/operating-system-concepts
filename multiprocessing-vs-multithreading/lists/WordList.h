struct WordLinkedList *createWordList();
void destroyWordLinkedList( struct WordLinkedList *wordList);
void addWord( struct WordLinkedList *wordList, char* word);
struct Node *getHeadNode( struct WordLinkedList *wordList);
void sort();

struct WordLinkedList 
{
    struct LinkedList* list;
};