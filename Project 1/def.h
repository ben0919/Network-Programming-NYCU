#ifndef DEF_H
#define DEF_H

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <iostream>
#include <vector>
#include <map>
#include <queue>
#include <string>
#include <cstring>

#define MAX_INPUT_LEN 15001
#define MAX_COMMAND_LEN 257
#define MAX_PIPE_NUM 1001
#define MAX_PROCESS_NUM 512

#define PIPE_READ 0
#define PIPE_WRITE 1

#define PIPE 0
#define NUMBERED_PIPE 1
#define REDIRECT 2

using namespace std;

extern string line;
extern vector<string> input_argv;
extern map<unsigned int, int*> numbered_pipes;
extern unsigned int line_count;

#endif