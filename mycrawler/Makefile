OBJS    = mycrawler.o mycrawler_helping.o threadpool.o queue.o
SOURCE  = mycrawler.c mycrawler_helping.c threadpool.c queue.c
HEADER  = mycrawler.h threadpool.h queue.h protocol_defines.h
OUT     = mycrawler   
CC      = gcc
FLAGS   = -g -c -Wall -lpthread
# -g option enables debugging mode 
# -c flag generates object code for separate files

all: $(OBJS)
	$(CC) -g $(OBJS) -o $(OUT) -lpthread

# create/compile the individual files >>separately<< 
mycrawler.o: mycrawler.c
	$(CC) $(FLAGS) mycrawler.c

mycrawler_helping.o: mycrawler_helping.c
	$(CC) $(FLAGS) mycrawler_helping.c

threadpool.o: threadpool.c
	$(CC) $(FLAGS) threadpool.c

queue.o: queue.c
	$(CC) $(FLAGS) queue.c

# clean house
clean:
	rm -f $(OBJS) $(OUT)

# do a bit of accounting
count:
	wc $(SOURCE) $(HEADER)
