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

#include "queue.h"
#include "myhttpd.h"
#include "protocol_defines.h"

Threadpool* threadpool_Init(int serving_port, char* root_dir) {
    Threadpool *thrpool = malloc(sizeof(Threadpool));

    thrpool->shutdown = 0;
    thrpool->queue = Queue_create();
    thrpool->serving_port = serving_port;
    strcpy(thrpool->root_dir, root_dir);
    //thrpool->root_dir = root_dir;
    int errno;
    if ((errno=pthread_mutex_init(&thrpool->mtx, NULL))) {
        perror("mutex init");
        return NULL;
    }
    if ((errno=pthread_mutex_init(&thrpool->mtx_pages, NULL))) {
        perror("mutex init");
        return NULL;
    }
    if ((errno=pthread_mutex_init(&thrpool->mtx_bytes, NULL))) {
        perror("mutex init");
        return NULL;
    }
    if ((errno=pthread_cond_init(&thrpool->cond, NULL))) {
        perror("cond init");
        return NULL;
    }
    return thrpool;
}


void queue_add(Threadpool* thrpool, int value){
    printf("about to add %d\n", value);
    pthread_mutex_lock(&thrpool->mtx);

    //Add element normally
    Queue_insert (thrpool->queue, value);

    pthread_mutex_unlock(&thrpool->mtx);

    //Signal waiting threads
    pthread_cond_signal(&thrpool->cond);
}

int queue_get(Threadpool* thrpool){
    int value;
    pthread_mutex_lock(&thrpool->mtx);

    //Wait for element to become available
    while (Queue_isEmpty(thrpool->queue)) pthread_cond_wait(&thrpool->cond, &thrpool->mtx);
    
    //We have an element. Pop it normally and return it in val_r
    value = Queue_removeData(thrpool->queue);

    pthread_mutex_unlock(&thrpool->mtx);
    printf("got %d\n", value);

    return value;
}

void *thr_routine(void* arg) {
    Threadpool* thrpool = (Threadpool*)arg;
    do {
        int newsock = queue_get(thrpool);
        char** request = malloc (100*sizeof(char*));
        for (int i=0; i<100; i++) request[i] = malloc (1000000);
        int lines = readHTTPrequest(newsock, request);
        while (lines) {
            //printf("pointer = %p\n", request[0]);
            printf("request = %s, lines = %d\n", request[0], lines);
            if (request[0] && request[0]!='\0') {
                resolveRequest (request[0], thrpool->root_dir, newsock);
                printf("request[0] = %s\n", request[0]);
            }
            else {
                printf("NULL REQUEST\n");
                return NULL;
            }
            lines = readHTTPrequest(newsock, request);
        }
        for (int i=0; i<100; i++) free(request[i]);
        free (request);
        close(newsock); //parent closes socket to client //must be closed before it gets re-assigned
    }while(!thrpool->shutdown);
    printf("thread is about to exit\n");

    pthread_exit(NULL);
}
int readHTTPrequest(int newsock, char** request) {
    //allocate memory for http request
    //char** request = malloc (100*sizeof(char*));
    int lines=0, bytes;
    while ((bytes=readLine(newsock, request[lines])>0)) lines++;
    if (bytes==-1) perror_exit ("read HTTP request");
    printf("request = %s\n", request[0]);
    return lines;
}

int readLine (int fd, char* line) {
    int i,error;
    char current_char;

    if (!line) return 0;

    for (i=0; (error=read(fd, &current_char, 1))>0 && current_char!='\n'; i++)
        *(line+i)=current_char;

    if (error==-1) perror_exit ("read line from file descriptor");

    *(line+i)='\0';
    return i>1; //return 1 if line has characters or 0 if not
}
void resolveRequest (const char* req, const char* root_dir, int newsock) {
    printf("req = %s\n", req);
    char method[10], path[1000];
    sscanf(req, "%s %s", method, path);
    printf("method = %s and path = %s\n", method, path);
    if (!strcmp(method, "GET")) {
        char* response = HTTP_Response (path, root_dir);
        //puts(response);
        if (write(newsock, response, strlen(response)) < 0) perror_exit("write"); //Reply
        //free(response);
    }
    else {
        printf("Not a GET method\n");
        sleep(2);
    }
}

char* HTTP_Response (const char* path, const char* root_dir) {
    printf("path = %s, root_dir = %s\n", path, root_dir);
    char* temp_path = malloc ((strlen(path)+1)*sizeof(char));
    char* temp_rootdir = malloc (10000);
    strcpy (temp_path, path); strcpy(temp_rootdir, root_dir);
    char* server_path = strcat (temp_rootdir, temp_path);
    printf("server_path = %s\n", server_path);

    int fd = open (server_path, O_RDONLY);
    free(temp_path); free(temp_rootdir);

    if (fd==-1) {
        if (errno==ENOENT || errno==ENOTDIR) return notExistsResponse();
        if (errno==EACCES) return noPermissionsResponse();
        perror_exit("open");
    }
    return OK_Response(fd);
}

char* OK_Response(int fd) {
    char* data=malloc(MAX_BYTES+1);

    int i, bytes=0, error;
    while ((error=read(fd, data+bytes, 1)==1)) bytes++;
    if (error==-1) perror_exit("read from file");
    data[bytes]='\0';

    printf("read %d bytes, \n", bytes);

    char *response = malloc ((MAX_BYTES+1000)*sizeof(char));
    sprintf(response, "HTTP/1.1 200 OK\nDate:\nServer: %s\nContent-Length: %d\nContent-Type: text/html\nConnection: Closed\n\r\n%s", SERVER, bytes, data);
    free(data);
    //return NULL;
    return response;
}

char* notExistsResponse(void) {
    int data_len = strlen(NOT_EXISTS_MSG);
    char *response = malloc (1000*sizeof(char));
    sprintf(response, "HTTP/1.1 404 Not Found\nDate:\nServer: %s\nContent-Length: %d\nContent-Type: text/html\nConnection: Closed\n\r\n%s", SERVER, data_len, NOT_EXISTS_MSG);
    return response;
}

char* noPermissionsResponse(void) {
    int data_len = strlen(NO_PERMISSIONS_MSG);
    char *response = malloc (1000*sizeof(char));
    sprintf(response, "HTTP/1.1 403 Forbidden\nDate:\nServer: %s\nContent-Length: %d\nContent-Type: text/html\nConnection: Closed\n\r\n%s", SERVER, data_len, NO_PERMISSIONS_MSG);
    return response;
}

void *createServingSocket (void* arg) {
    Threadpool* thrpool = (Threadpool*)arg;

    int sock, newsock;
    struct sockaddr_in server, client;
    socklen_t clientlen = sizeof(client);

    struct sockaddr *serverptr=(struct sockaddr *)&server;
    struct sockaddr *clientptr=(struct sockaddr *)&client;
    //struct hostent *rem;

    //Create socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) perror_exit("socket");

    thrpool->serving_socket = sock;

    server.sin_family = AF_INET;       //Internet domain
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_port = htons(thrpool->serving_port);

    //Bind socket to address
    if (bind(sock, serverptr, sizeof(server)) < 0) perror_exit("bind");

    //Listen for connections
    if (listen(sock, 5) < 0) perror_exit("listen");
    printf("Listening for connections to serving port %d\n", thrpool->serving_port);

    while (!thrpool->shutdown) {
        //accept connection
        if ((newsock = accept(sock, clientptr, &clientlen)) < 0) perror_exit("accept");
        //Find client's address
        //if ((rem = gethostbyaddr((char *) &client.sin_addr.s_addr, sizeof(client.sin_addr.s_addr), client.sin_family)) == NULL) {
        //herror("gethostbyaddr"); exit(1);}
        //printf("Accepted connection from %s\n", rem->h_name);
        printf("Accepted serving connection\n");

        //add new socket's file descriptor in queue
        queue_add(thrpool, newsock);
    }
    printf("About to exit serving socket function\n");
    pthread_exit(NULL);
}


int createCommandSocket (int command_port, Threadpool* thrpool, int thr_no) {
    time_t start=time(NULL);
    int sock, newsock;
    struct sockaddr_in server, client;
    socklen_t clientlen = sizeof(client);

    struct sockaddr *serverptr=(struct sockaddr *)&server;
    struct sockaddr *clientptr=(struct sockaddr *)&client;
    //struct hostent *rem;

    //Create socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) perror_exit("socket");

    server.sin_family = AF_INET;       //Internet domain
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_port = htons(command_port);

    //Bind socket to address
    if (bind(sock, serverptr, sizeof(server)) < 0) perror_exit("bind");

    //Listen for connections
    if (listen(sock, 5) < 0) perror_exit("listen");
    printf("Listening for command connections to port %d\n", command_port);

    //accept connection
    if ((newsock = accept(sock, clientptr, &clientlen)) < 0) perror_exit("accept");
    //Find client's address
    //if ((rem = gethostbyaddr((char *) &client.sin_addr.s_addr, sizeof(client.sin_addr.s_addr), client.sin_family)) == NULL) {
    //herror("gethostbyaddr"); exit(1);}
    //printf("Accepted connection from %s\n", rem->h_name);
    printf("Accepted command connection\n");
    
    enterCLI(start, newsock);

    thrpool->shutdown=1;
    int shuterror = shutdown (thrpool->serving_socket, SHUT_RDWR);
    if (shuterror) {
        perror("shutdown serving socket");
        exit(1);
    }

    for (int i=0; i<thr_no; i++) queue_add(thrpool, -1); //empty queue
    
    printf("Closing command connection.\n");
    close(newsock);   //Close socket

    printf("About to exit command socket function\n");
    return 1;
}

int enterCLI (time_t start, int newsock) {
    char* line=malloc(1000);
    readLine(newsock, line);
    if (!line) {fprintf(stderr, "Error while reading from command socket\n"); return 0;}
    while (strcmp(line, "SHUTDOWN\r")!=0) {
        //printf("strlen = %ld\n", strlen(line));
        printf("line = %s\n", line);

        if (!strcmp(line, "STATS\r")) {
            char reply[100];
            //double seconds = (double)(clock()-thrpool->begin)/CLOCKS_PER_SEC;
            time_t end = time(NULL);

            time_t elapsed = end-start;
            //printf("elapsed time = %ld\n", elapsed);
            long h = elapsed / 3600;
            long m = (elapsed / 60) % 60;
            long s = elapsed % 60;
            sprintf(reply, "Server up for %ld:%ld:%ld\n", h, m, s);
            //write(newsock, reply, strlen(reply)+1);

            //Reply
            if (write(newsock, reply, strlen(reply)) < 0) perror_exit("write");
        }

        readLine(newsock, line);
        if (!line) {fprintf(stderr, "Error while reading from command socket\n"); return 0;}
    }
    free (line);
    return 1;
}

void perror_exit(char *message) {
    perror(message);
    exit(EXIT_FAILURE);
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
