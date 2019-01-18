#include <stdio.h>
#include <unistd.h>
#include <ctype.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <sys/wait.h>        /* sockets */
#include <sys/types.h>       /* sockets */
#include <sys/socket.h>      /* sockets */
#include <netinet/in.h>      /* internet sockets */
#include <netdb.h>           /* gethostbyaddr */
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>
#include <dirent.h>

#include "threadpool.h"

Threadpool* threadpool_Init(int port, const char* save_dir, const char* host, const char* starting_URL, int socket) {
    Threadpool *thrpool = malloc(sizeof(Threadpool));

    thrpool->socket = socket;
    thrpool->queue = Queue_create();
    Queue_insert(thrpool->queue, starting_URL);
    thrpool->existsQueue = Queue_create();
    Queue_insert(thrpool->existsQueue, starting_URL);
    thrpool->begin = clock();
    thrpool->port = port;
    strcpy(thrpool->save_dir, save_dir);
    strcpy(thrpool->host, host);

    if (mkdir(save_dir,0755)==-1) perror_exit("mkdir");
    strcpy(thrpool->save_dir, save_dir);

    thrpool->get_repeats=0;
    thrpool->add_repeats=1;

    //thrpool->root_dir = root_dir;
    int errno;
    if ((errno=pthread_mutex_init(&thrpool->mtx, NULL))) {
        perror("mutex init");
        return NULL;
    }
    if ((errno=pthread_mutex_init(&thrpool->mtx_add, NULL))) {
        perror("mutex init");
        return NULL;
    }
    if ((errno=pthread_mutex_init(&thrpool->mtx_get, NULL))) {
        perror("mutex init");
        return NULL;
    }
    if ((errno=pthread_mutex_init(&thrpool->mtx_existsQueue, NULL))) {
        perror("mutex init");
        return NULL;
    }
    if ((errno=pthread_mutex_init(&thrpool->mtx_existsQueue2, NULL))) {
        perror("mutex init");
        return NULL;
    }
    if ((errno=pthread_cond_init(&thrpool->cond, NULL))) {
        perror("cond init");
        return NULL;
    }
    return thrpool;
}


void threadpool_add(Threadpool* thrpool, const char* value){

    pthread_mutex_lock(&thrpool->mtx);

    //Add element normally
    Queue_insert (thrpool->queue, value);

    pthread_mutex_unlock(&thrpool->mtx);

    //Signal waiting threads
    pthread_cond_signal(&thrpool->cond);
}

char* threadpool_get(Threadpool* thrpool){
    pthread_mutex_lock(&thrpool->mtx);

    //Wait for element to become available
    while (Queue_isEmpty(thrpool->queue)) pthread_cond_wait(&thrpool->cond, &thrpool->mtx);
    
    //We have an element. Pop it normally and return it in val_r
    char* value = Queue_removeData(thrpool->queue);

    pthread_mutex_unlock(&thrpool->mtx);

    return value;
}

void *thr_routine(void* arg) {
    Threadpool* thrpool = (Threadpool*)arg;
    int sock = connectToServer (thrpool->host, thrpool->port);

    do {
        char* path = threadpool_get(thrpool);
        printf("path = %s\n", path);
        pthread_mutex_lock(&thrpool->mtx_get);
        thrpool->get_repeats++;
        pthread_mutex_unlock(&thrpool->mtx_get);
        
        sendHTTP_GET (thrpool->host, path, sock);
        char* line = malloc (1000);
        char temp[100];
        int status;
        if (!readLine(sock, line)) exit(1);
        sscanf(line, "%s %d", temp, &status);
        if (status!=200) {
            printf("Status %d!\n", status);
            exit(1);
        }
        int len=-1, i=0;
        while(readLine(sock, line)) {
            //puts(line);
            char* startt;
            if ((startt=strstr(line, "Content-Length: "))) {
                startt+=16;
                len = atoi(startt);
            }
        }
        free(line);
        usleep(1000);

        char* msg=malloc((len+1)*sizeof(char));
        if (read(sock, msg, len)<0) perror_exit("read message");
        msg[len]='\0';

        saveToFile (path, msg, thrpool->save_dir);

        findLink(thrpool, msg, &thrpool->add_repeats);
        //puts(msg);
        free(msg);
    }while(thrpool->get_repeats!=thrpool->add_repeats);
    while(!Queue_isEmpty(thrpool->existsQueue)) Queue_removeData(thrpool->existsQueue);
    close(sock); //Close socket and exit

    pthread_exit(NULL);
}

int connectToServer(const char* host, int port) {
    int sock;
    struct sockaddr_in server;
    struct sockaddr *serverptr = (struct sockaddr*)&server;
    struct hostent *rem;

    //Create socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) perror_exit("socket");

    //Find server address
    if ((rem = gethostbyname(host)) == NULL) {
       herror("gethostbyname"); exit(1);
    }

    server.sin_family = AF_INET; //Internet domain
    memcpy(&server.sin_addr, rem->h_addr, rem->h_length);
    server.sin_port = htons(port);         //Server port

    //Initiate connection
    if (connect(sock, serverptr, sizeof(server)) < 0) perror_exit("connect");
    printf("Connecting to %s port %d\n", host, port);
    return sock;
}

int saveToFile (const char* path, const char* msg, const char* save_dir) {
    char* start=strrchr(path, '/');
    
    char temp[1000];
    strcpy(temp, save_dir);
    strcat(temp, start);

    FILE* fd = fopen(temp, "a");
    if (!fd) perror_exit ("fopen file to write message");
    fprintf(fd, "%s", msg);
    fclose(fd);
}


void findLink (Threadpool* thrpool, const char* msg, int* times) {
    char* tempbuf = malloc ((strlen(msg)+1)*sizeof(char));
    if (!tempbuf) perror_exit ("malloc temp buffer");
    strcpy (tempbuf, msg);
    
    char* start = strstr (tempbuf, "<a href=\"");
    if (start) start+=9;
    else {
        free (tempbuf);
        return;
    }
    char* end = strstr (tempbuf, ".html\">");
    if (end) end[5] = '\0';
    else {
        printf("Null end\n");
        free (tempbuf);
        return;
    }
    
    pthread_mutex_lock(&thrpool->mtx_existsQueue2);
    if (!Queue_itemExists(thrpool->existsQueue, start)){
        pthread_mutex_unlock(&thrpool->mtx_existsQueue2);
        threadpool_add (thrpool, start);
        pthread_mutex_lock(&thrpool->mtx_add);
        (*times)++;
        pthread_mutex_unlock(&thrpool->mtx_add);
    }
    else pthread_mutex_unlock(&thrpool->mtx_existsQueue2);
    
    pthread_mutex_lock(&thrpool->mtx_existsQueue);
    Queue_insert(thrpool->existsQueue, start);
    pthread_mutex_unlock(&thrpool->mtx_existsQueue);
    findLink (thrpool, end+6, times);
    free (tempbuf);
}

pthread_t* createThreads (int thr_no, Threadpool* thrpool){
    pthread_t *tids;
    int err;

    if ((tids = malloc(thr_no * sizeof(pthread_t))) == NULL) {
        perror("malloc");
        return NULL;
    }

    for (int i=0; i<thr_no; i++) {
        err = pthread_create(tids+i, NULL, thr_routine, (void*)thrpool);
        if (err) {
            perror2("pthread_create", err);
            printf("mphka sto error\n");
            return NULL;
        }
    }

    return tids;
}

int close_threads (pthread_t* tids, int thr_no) {
    int err;
    for (int i=0; i<thr_no; i++) {
        if ((err = pthread_join(*(tids+i), NULL))) {
        //Wait for thread termination
            perror2("pthread_join", err);
            exit(1);
        }
    }
    printf("all %d threads have terminated\n", thr_no);
    free (tids);
    return 1;
}

int threadpool_Destroy(Threadpool** thrpool) {
    int ok;
    ok=Queue_destroy(&((*thrpool)->queue));
    ok=Queue_destroy(&((*thrpool)->existsQueue));
    if (ok) {free (*thrpool); *thrpool=NULL;}
    return ok;
}
