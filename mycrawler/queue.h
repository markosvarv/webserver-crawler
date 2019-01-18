#define MAX_PATH 1000

typedef struct Queue* Queue_handler;

Queue_handler Queue_create (void);

int Queue_isEmpty(Queue_handler queue);

int Queue_itemExists(Queue_handler queue, const char* item);

void Queue_insert(Queue_handler queue, const char* k);

char* Queue_removeData(Queue_handler queue);

int Queue_destroy(Queue_handler* queue);
