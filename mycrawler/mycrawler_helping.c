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

#include "mycrawler.h"
#include "threadpool.h"

int sendHTTP_GET (const char* host, const char* path, int socket){
    char request[1000];
    sprintf(request, "GET %s HTPP/1.1\nUser-Agent: Mozzila/4.0 (Compatible)\nHost: %s\nAccept-Language: en-us\nAccept-Encoding: gzip, feflate\nConnection: Keep-Alive\n\r\n", path, host);
    //printf("request = %s\n", request);
    if (write(socket, request, strlen(request)) < 0) perror_exit("write");
    return 1;
}

int readLine (int fd, char* line) {
    int i,error;
    char current_char;

    for (i=0; (error=read(fd, &current_char, 1))>0 && current_char!='\n'; i++){
        *(line+i)=current_char;
    }
    if (error==-1) perror_exit ("read line from file descriptor");

    *(line+i)='\0';
    return i>1; //return 1 if line has characters or 0 if not
}

void perror_exit(char *message) {
    perror(message);
    exit(EXIT_FAILURE);
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
    printf("Accepted command connection\n");
    
    enterCLI(start, newsock);

    thrpool->shutdown=1;
    int shuterror = shutdown (thrpool->socket, SHUT_RDWR);
    if (shuterror) {
        perror("shutdown serving socket");
        exit(1);
    }

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
        printf("line = %s\n", line);

        if (!strcmp(line, "STATS\r")) {
            char reply[100];
            time_t end = time(NULL);
            time_t elapsed = end-start;
            long h = elapsed / 3600;
            long m = (elapsed / 60) % 60;
            long s = elapsed % 60;
            sprintf(reply, "Server up for %ld:%ld:%ld\n", h, m, s);
            //Reply
            if (write(newsock, reply, strlen(reply)) < 0) perror_exit("write");
        }

        readLine(newsock, line);
        if (!line) {fprintf(stderr, "Error while reading from command socket\n"); return 0;}
    }
    free (line);
    return 1;
}
