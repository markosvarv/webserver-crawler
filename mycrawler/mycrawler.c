#include <stdio.h>
#include <unistd.h>
#include <ctype.h>
#include <stdlib.h>
#include <sys/types.h>	     /* sockets */
#include <sys/socket.h>	     /* sockets */
#include <netinet/in.h>	     /* internet sockets */
#include <unistd.h>          /* read, write, close */
#include <netdb.h>	         /* gethostbyaddr */
#include <stdlib.h>	         /* exit */
#include <string.h>	         /* strlen */
#include <pthread.h>
#include "mycrawler.h"
#include "threadpool.h"
#include "protocol_defines.h"
void perror_exit(char *message);
/*
int createSockets(const char* host, int port, const char* path) {
    int sock;
    //char readbuf[MAX_BYTES];
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
    //char* path = threadpool_get(thrpool);
    sendHTTP_GET (host, path, sock);

    char* line = malloc (1000);
    int len=-1;
    while(readLine(sock, line)) {
        puts(line);
        char* start;
        if ((start=strstr(line, "Content-Length: "))) {
            start+=16;
            len = atoi(start);
        }
    }
    printf("len = %d\n", len);
    free(line);

    char* msg=malloc((len+1)*sizeof(char));
    if (read(sock, msg, len)<0) perror_exit("read");
    msg[len]='\0';
    findLink(msg);
    //puts(msg);
    free(msg);

    close(sock); //Close socket and exit

    return 1;
}			     

int findLink (const char* msg) {
    char* tempbuf = malloc ((strlen(msg)+1)*sizeof(char));
    if (!tempbuf) perror_exit ("malloc temp buffer");
    strcpy (tempbuf, msg);
    
    char* start = strstr (tempbuf, "<a href=\"");
    if (start) start+=9;
    else {
        free (tempbuf);
        return 0;
    }
    char* end = strstr (tempbuf, ".html\">");
    if (end) end[5] = '\0';
    else {
        printf("Null end\n");
        free (tempbuf);
        return 0;
    }
        
    printf("start = %s\n", start);
    //threadpool_add (thrpool, start);
    findLink (end+6);
    free (tempbuf);
}
*/

int createSocket(const char* host, int port) {
    return -1;
}


int main(int argc, char *argv[]) {
    int opt, pvalue=0, cvalue=0, tvalue=0;
    char *hvalue=NULL, *dvalue=NULL;

    if (argc!=12) {
        fprintf(stderr, "Need 12 arguments. Right format is: ./mycrawler -h host_or_IP -p port -c command_port -t num_of_threads -d save_dir starting_URL\n");
        return 1;
    }

    while ((opt = getopt(argc, argv, "h:p:c:t:d:")) != -1) {
        switch (opt) {
        case 'h':
            hvalue = optarg;
            break;
        case 'p':
            pvalue = atoi(optarg);
            break;
        case 'c':
            cvalue = atoi(optarg);
            break;
        case 't':
            tvalue = atoi(optarg);
            break;
        case 'd':
            dvalue = optarg;
            break;

        case '?':
            if (isprint (optopt))
                fprintf (stderr, "Unknown option `-%c'.\n", optopt);
            else
                fprintf (stderr, "Unknown option character `\\x%x'.\n", optopt);
            exit(1);
        default:
            abort ();
        }
    }
    //printf("optind = %d, argv = %s\n", optind, argv[optind]);
    //printf("hvalue = %s; pvalue=%d; cvalue=%d, tvalue=%d, dvalue=%s\n", hvalue, pvalue, cvalue, tvalue, dvalue);
    if (!dvalue || !pvalue || !cvalue || !tvalue || !hvalue) {
        fprintf (stderr, "Wrong value(s)\n");
        return 1;
    }
    int socket = createSocket (hvalue, pvalue);
    //create threadpool
    Threadpool* thrpool = threadpool_Init(pvalue, dvalue, hvalue, argv[optind], socket);
    if (!thrpool) {
        fprintf(stderr, "Error while creating threadpool. Program will end\n");
        return 1;
    }

    pthread_t* tids = createThreads (tvalue, thrpool);
    int err;
    pthread_t thr;

    //createCommandSocket (cvalue, thrpool, tvalue);

    printf("WAITING TO TERMINATE ALL THREADS\n");
    close_threads (tids, tvalue);
    printf("THREADS TERMINATED, WAITING TO TERMINATE serving socket thread\n");
    threadpool_Destroy(&thrpool);

    free (tids);
}
