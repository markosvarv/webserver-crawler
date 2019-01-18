#include "queue.h"
#define MAX_HTTP_GET 10000

#define perror2(s, e) fprintf(stderr, "%s: %s\n", s, strerror(e))

typedef struct Threadpool{
    Queue_handler queue;
    Queue_handler existsQueue;
    int shutdown;
    int socket;
    int port;
    char save_dir[1000];
    char host[1000];
    int sending_pages;
    clock_t begin;
    int get_repeats;
    int add_repeats;
    pthread_mutex_t mtx;
    pthread_mutex_t mtx_get;
    pthread_mutex_t mtx_add;
    pthread_mutex_t mtx_existsQueue;
    pthread_mutex_t mtx_existsQueue2;
    pthread_cond_t cond;
}Threadpool;


Threadpool* threadpool_Init(int port, const char* save_dir, const char* host, const char* starting_URL, int socket);
void threadpool_add(Threadpool* thrpool, const char* value);
char* threadpool_get(Threadpool* thrpool);
void *thr_routine(void* arg);
pthread_t* createThreads (int thr_no, Threadpool* thrpool);
int close_threads (pthread_t* tids, int thr_no);
void perror_exit(char *message);
int readLine (int fd, char* line);
int sendHTTP_GET (const char* host, const char* path, int socket);
void findLink (Threadpool* thrpool, const char* msg, int* times);
int threadpool_Destroy(Threadpool** thrpool);
int connectToServer(const char* host, int port);
int saveToFile (const char* path, const char* msg, const char* save_dir);
