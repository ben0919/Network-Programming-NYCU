#ifndef COMMAND_H
#define COMMAND_H

#include "def.h"

class command {
public:
    command(int type, vector<string> argv, bool before, bool after, bool numbered_pipe_before, bool numbered_pipe_after, bool pipe_stderr);
    command(int type, vector<string> argv, string file_name, bool before, bool after, bool numbered_pipe_before, bool numbered_pipe_after, bool pipe_stderr);
    command(int type, vector<string> argv, int pipe_number, bool before, bool after, bool numbered_pipe_before, bool numbered_pipe_after, bool pipe_stderr);
    ~command();
    int type;
    char *name;
    char **argv;
    int argv_length;
    char *file_name;
    int pipe_number;
    bool pipe_before;
    bool pipe_after;
    bool numbered_pipe_before;
    bool numbered_pipe_after;
    bool pipe_stderr;
};

#endif