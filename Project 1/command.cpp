#include "def.h"
#include "command.h"


command::command(int type, vector<string> argv, bool before, bool after, bool numbered_pipe_before, bool numbered_pipe_after, bool pipe_stderr) {
    this->type = type;
    this->argv_length = argv.size() + 1;
    this->argv = new char*[this->argv_length];
    for (int i = 0; i < argv.size(); ++i) {
        char temp[MAX_COMMAND_LEN];
        strcpy(temp, argv[i].c_str());
        this->argv[i] = strdup(temp);
    }
    this->argv[argv.size()] = NULL;
    this->file_name = nullptr; 
    this->pipe_number = 0;
    this->name = this->argv[0];
    this->pipe_before = before;
    this->pipe_after = after;
    this->numbered_pipe_before = numbered_pipe_before;
    this->numbered_pipe_after = numbered_pipe_after;
    this->pipe_stderr = pipe_stderr;
}

command::command(int type, vector<string> argv, string file_name, bool before, bool after, bool numbered_pipe_before, bool numbered_pipe_after, bool pipe_stderr) {
    this->type = type;
    this->argv_length = argv.size() + 1;
    this->argv = new char*[this->argv_length];
    for (int i = 0; i < argv.size(); ++i) {
        char temp[MAX_COMMAND_LEN];
        strcpy(temp, argv[i].c_str());
        this->argv[i] = strdup(temp);
    }
    this->argv[argv.size()] = NULL;

    char temp[MAX_COMMAND_LEN];
    strcpy(temp, file_name.c_str());
    this->file_name = strdup(temp);

    this->pipe_number = 0;
    this->name = this->argv[0];
    this->pipe_before = before;
    this->pipe_after = after;
    this->numbered_pipe_before = numbered_pipe_before;
    this->numbered_pipe_after = numbered_pipe_after;
    this->pipe_stderr = pipe_stderr;
}

command::command(int type, vector<string> argv, int pipe_number, bool before, bool after, bool numbered_pipe_before, bool numbered_pipe_after, bool pipe_stderr) {
    this->type = type;
    this->argv_length = argv.size() + 1;
    this->argv = new char*[this->argv_length];
    for (int i = 0; i < argv.size(); ++i) {
        char temp[MAX_COMMAND_LEN];
        strcpy(temp, argv[i].c_str());
        this->argv[i] = strdup(temp);
    }
    this->argv[argv.size()] = NULL;
    this->file_name = nullptr; 
    this->pipe_number = pipe_number;
    this->name = this->argv[0];
    this->pipe_before = before;
    this->pipe_after = after;
    this->numbered_pipe_before = numbered_pipe_before;
    this->numbered_pipe_after = numbered_pipe_after;
    this->pipe_stderr = pipe_stderr;
}

command::~command(){
    for(int i = 0; i < this->argv_length; ++i) {
        delete[] this->argv[i];
    }
    delete[] this->argv;
}
