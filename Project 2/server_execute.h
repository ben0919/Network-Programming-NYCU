#ifndef SERVER_EXECUTE_H
#define SERVER_EXECUTE_H
#include "def.h"
#include "client.h"
#include "command.h"
#include "server_utils.h"

int do_server_build_in_command(vector<string> argv, client* client_, string line);
void exec_redirect(vector<string> argv, string file_name);
unsigned int server_execute_commands(vector<command*> commands, client* client_, string line);
void execute_pipe(command *command_, int *pipe_fd, client* client_, string line);
void execute_redirect(command *command_, client* client_, string line);
void pipe_child_in(command *command_);
void pipe_child_out(command *command_, int *pipe_fd);
void pipe_parent(command *command_);
void numbered_pipe_parent(map<unsigned int, int*> &numbered_pipes, unsigned int line_count);
void numbered_pipe_child_in(command *command_, map<unsigned int, int*> &numbered_pipes, unsigned int line_count);
void user_pipe_in(command *command_, client *client_);
void user_pipe_out(command *command_, client *client_);
void user_pipe_parent(command *command_, client *client_);
void wait_pid(pid_t pid);
#endif