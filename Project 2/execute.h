#ifndef EXECUTE_H
#define EXECUTE_H

#include "def.h"
#include "command.h"

int do_build_in_command(vector<string> argv); 
//return 1 if a built-in command is done, -1 if a built-in command failed, 0 if the command is not a built-in command.
unsigned int execute_commands(vector<command*> commands, map<unsigned int, int*> &numbered_pipes, unsigned int line_count);
void execute_pipe(command *command_, int *pipe_fd, map<unsigned int, int*> &numbered_pipes, unsigned int line_count);
void execute_redirect(command *command_, map<unsigned int, int*> &numbered_pipes, unsigned int line_count);
void pipe_child_in(command *command_);
void pipe_child_out(command *command_, int *pipe_fd);
void pipe_parent(command *command_);
void numbered_pipe_parent(map<unsigned int, int*> &numbered_pipes, unsigned int line_count);
void numbered_pipe_child_in(command *command_, map<unsigned int, int*> &numbered_pipes, unsigned int line_count);
void wait_pid(pid_t pid);
#endif