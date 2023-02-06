#ifndef EXECUTE_H
#define EXECUTE_H

#include "def.h"
#include "command.h"

int do_build_in_command(vector<string> argv); 
//return 1 if a built-in command is done, -1 if a built-in command failed, 0 if the command is not a built-in command.

void exec_redirect(vector<string> argv, string file_name);
void execute_commands(vector<command*> commands);
void execute_pipe(command *command_, int *pipe_fd, queue<int*> &pipes);
void execute_redirect(command *command_, queue<int*> &pipes);
void pipe_child_in(command *command_, queue<int*> &pipes);
void pipe_child_out(command *command_, int *pipe_fd);
void pipe_parent(command *command_, queue<int*> &pipes);
void numbered_pipe_parent();
void numbered_pipe_child_in(command *command_);
void wait_pid(pid_t pid);
#endif