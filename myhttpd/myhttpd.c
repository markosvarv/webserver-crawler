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
#include <time.h>

#include "myhttpd.h"
#include "queue.h"

int main(int argc, char *argv[]) {
    int opt, pvalue=0, cvalue=0, tvalue=0;
    char *dvalue=NULL;

    if (argc!=9) {
        fprintf(stderr, "Need 4 arguments. Right format is: ./myhttpd -p serving_port -c command_port -t num_of_threads -d root_dir\n");
        return 1;
    }

    while ((opt = getopt(argc, argv, "p:c:t:d:")) != -1) {
        switch (opt) {
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
    printf("dvalue = %s; pvalue=%d; cvalue=%d, tvalue=%d\n", dvalue, pvalue, cvalue, tvalue);
    if (!dvalue || !pvalue || !cvalue || !tvalue) {
        fprintf (stderr, "Wrong value(s)\n");
        printf("Try again running: ./jocExecutor -d docfile -w numWorkers\n");
        return 1;
    }

    //create threadpool
    Threadpool* thrpool = threadpool_Init(pvalue, dvalue);
    if (!thrpool) {
        fprintf(stderr, "Error while creating threadpool. Program will end\n");
        return 1;
    }

    pthread_t* tids = createThreads (tvalue, thrpool);
    int err;
    pthread_t thr;

    if ((err = pthread_create(&thr, NULL, createServingSocket, (void*)thrpool))) { //New thread
        perror2("pthread_create", err);
        exit(1);
    } 
    createCommandSocket (cvalue, thrpool, tvalue);

    int status;

    printf("WAITING TO TERMINATE ALL THREADS\n");
    close_threads (tids, tvalue);
    printf("THREADS TERMINATED, WAITING TO TERMINATE serving socket thread\n");
    if ((err = pthread_join(thr, (void **) &status))) {  //Wait for thread
        perror2("pthread_join", err); //termination
        exit(1);
    }
    Queue_destroy(&thrpool->queue);
    free (thrpool);
    free (tids);
}



