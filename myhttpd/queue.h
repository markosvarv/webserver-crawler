typedef struct Queue* Queue_handler;

Queue_handler Queue_create (void);

int Queue_peek(Queue_handler queue);

int Queue_isEmpty(Queue_handler queue);

int Queue_getSize(Queue_handler queue);

void Queue_insert(Queue_handler queue, int data);

int Queue_removeData(Queue_handler queue);

void Queue_destroy(Queue_handler* queue);
