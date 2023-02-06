#ifndef DEF_H
#define DEF_H

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <iostream>
#include <vector>
#include <map>
#include <queue>
#include <string>
#include <cstring>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <semaphore.h>
#include <sys/stat.h>
#include <errno.h>

#define MAX_INPUT_LEN 15001
#define MAX_COMMAND_LEN 257
#define MAX_PIPE_NUM 1001
#define MAX_PROCESS_NUM 512

#define PIPE_READ 0
#define PIPE_WRITE 1

#define PIPE 0
#define NUMBERED_PIPE 1
#define REDIRECT 2
#define USER_PIPE 3

using namespace std;


#endif