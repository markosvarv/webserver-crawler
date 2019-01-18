#define MAX_HTTP_GET 10000

#define SERVER "myhttp/1.0.0 (Ubuntu64)"

#include "queue.h"

#define perror2(s, e) fprintf(stderr, "%s: %s\n", s, strerror(e))

typedef struct Threadpool{
    Queue_handler queue;
    int shutdown;
    int serving_socket;
    char root_dir[1000];
    int served_pages;
    long bytes;
    int serving_port;
    pthread_mutex_t mtx;
    pthread_mutex_t mtx_pages;
    pthread_mutex_t mtx_bytes;
    pthread_cond_t cond;
}Threadpool;


Threadpool* threadpool_Init(int serving_port, char* root_dir);
pthread_t* createThreads (int thr_no, Threadpool* thrpool);
int readHTTPrequest(int newsock, char** request);
void resolveRequest (const char* req, const char* root_dir, int newsock);
char* HTTP_Response (const char* path, const char* root_dir);
char* OK_Response(int fd);
char* notExistsResponse(void);
char* noPermissionsResponse(void);
void perror_exit(char *message);
void *createServingSocket (void* arg);
//void createServingSocket (Threadpool* thrpool);
int createCommandSocket (int command_port, Threadpool* thrpool, int thr_no);
int close_threads (pthread_t* tids, int thr_no);
int readLine (int fd, char* line);
int enterCLI (time_t start, int newsock);
