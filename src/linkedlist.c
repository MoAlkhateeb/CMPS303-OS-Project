#include "headers.h"
#include "linkedlist.h"

node * createList(){
    node * dummyNode;
    dummyNode = malloc(sizeof(node));
    dummyNode->nodeLength = 0;
    dummyNode->nodeValue = 0;
    
    return dummyNode;
}

void addFront(node * head, int i){
    if (head == NULL) {
        fprintf(stderr, "Error: Invalid pointer\n");
        exit(EXIT_FAILURE);
    }
    node * newNode = initNode(i);
    newNode->next = head->next;
    head->next = newNode;
}

int getFront(node* head){
    if (head == NULL || head->next == NULL) {
        fprintf(stderr, "Error: List is empty\n");
        exit(EXIT_FAILURE);
    }
    return head->next->nodeValue;
}

void delete(node* head, int id){
    if(head==NULL || head->next==NULL){
        fprintf(stderr, "Error: List is empty\n");
        exit(EXIT_FAILURE);
    }
    node* current= head->next;
    node* prev= head;
    if (current->nodeValue->id == id && current->next == NULL) {
        free(current->nodeValue);
        free(current);
        head->next = NULL; // Update head to point to NULL
        return;
    }
    
    while(current!=NULL){
        if(current->nodeValue->id== id){
            prev->next= current->next;
            free(current->nodeValue);
            free(current);
            return;
        }
        prev=current;
        current= current->next;
    }
    fprintf(stderr,"Error: Node with id %d not found\n",id);
}

void printList(node * head){
    node * N;
    N = head;
    N = N->next;
    while (N != NULL) {
        printf("%d ", N->nodeValue);
        N = N->next;
    }
}

node * initNode(int i){
    node * newNode;
    newNode = malloc(sizeof(node));
    newNode->nodeValue = i;
    newNode->next = NULL;
    
    return newNode;
}

void destroy(node** headptr){
    if (*headptr == NULL) {
        return; // No need to destroy an already destroyed list
    }
    node * current = *headptr;
    node * next;
    while (current != NULL) {
        next = current->next;
        free(current);
        current = next;
    }
    *headptr = NULL; // Set head pointer to NULL after destruction
}