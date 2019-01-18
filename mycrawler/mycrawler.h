
int sendHTTP_GET (const char* host, const char* path, int socket);
int readLine (int fd, char* line);
void perror_exit(char *message);
//int createCommandSocket (int command_port, Threadpool* thrpool, int thr_no);
int enterCLI (time_t start, int newsock);
