#include "headers.h"
#include "process_control_block.c"

struct listNode {
	int nodeLength;
	pcb* nodeValue;
	struct listNode * next;
};

typedef struct listNode node;

node * createList();
void addFront(node * head, int i);
int getFront(node* head);
void delete(node* head, int id);
void printList(node * head);
node * initNode(int i);
void destroy(node** headptr);

