#include <stdio.h>
#include <stdlib.h>

#include "queue.h"

struct QNode {
    int key;
    struct QNode *next;
};
 
// The queue, front stores the front node of LL and rear stores ths
// last node of LL
typedef struct Queue {
    struct QNode *front, *rear;
    int size;
}Queue;
 
// A utility function to create a new linked list node.
struct QNode* newNode(int k){
    struct QNode *temp = malloc(sizeof(struct QNode));
    temp->key = k;
    temp->next = NULL;
    return temp; 
}
 
// A utility function to create an empty queue
Queue_handler Queue_create(void) {
    Queue_handler q = malloc(sizeof(Queue));
    q->front = q->rear = NULL;
    q->size = 0;
    return q;
}
 
// The function to add a key k to q
void Queue_insert(Queue_handler q, int k) {
    // Create a new LL node
    struct QNode *temp = newNode(k);
    q->size++;
 
    // If queue is empty, then new node is front and rear both
    if (q->rear == NULL){
       q->front = q->rear = temp;
       return;
    }
 
    // Add the new node at the end of queue and change rear
    q->rear->next = temp;
    q->rear = temp;
}
 
// Function to remove a key from given queue q
int Queue_removeData(Queue_handler q){
    // If queue is empty, return NULL.
    if (!q->front) return -1;
 
    // Store previous front and move front one node ahead
    struct QNode *temp = q->front;
    q->front = q->front->next;
 
    // If front becomes NULL, then change rear also as NULL
    if (!q->front) q->rear = NULL;
    q->size--;
    return temp->key;
}

int Queue_isEmpty(Queue_handler queue){
    return queue->size==0;
}

int Queue_getSize(Queue_handler queue){
    return queue->size;
}

void Queue_destroy(Queue_handler* queue) {
    struct QNode *current = (*queue)->front;
    struct QNode *nextnode=NULL;
    while (current) {
        nextnode = current->next;
        free (current);
        current = nextnode;
    }

    free(*queue);
    *queue=NULL;
}
