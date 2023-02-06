#ifndef NP_MULTI_PROC_H
#define NP_MULTI_PROC_H

#include "def.h"
#include "command.h"
#include "utils.h"

struct client {
    client(int pid_, int id_, int port_, int socketfd_, const char* name_, char* ip_){
        pid = pid_;
        id = id_;
        port = port_;
        socketfd = socketfd_;
        memset(name, 0, sizeof(name));
		memset(ip, 0, sizeof(ip));
        strcpy(name, name_);
        strcpy(ip, ip_);
    };
    int pid;
    int id;
    int port;
    int socketfd;
    char name[25];
    char ip[50];
};

struct shared_memory {
    struct client client_list[31];
    bool available_id[31];
    bool user_pipe_record[31][31];
    int user_pipe_in_id[31][31];
    int user_pipe_out_id[31];
    int user_pipe_from;
    int user_pipe_to;
    int user_num;
    char message[1025];
    sem_t sem;
};
int find_available_id();
void init_shared_momory();
void parentINT(int signo);
void childINT(int signo);
void recvUSR1(int signo);
void write_welcome_message(int fd);
void broadcast(string message);
void broadcast_login(int id);
void broadcast_logout(int id);
void write_prompt(int fd);
vector<command*> server_parse(vector<string> &input_argv);
int do_server_build_in_command(vector<string> argv, int id, string line);
unsigned int execute_commands(vector<command*> commands, int id, map<unsigned int, int*> &numbered_pipes, unsigned int line_count, string line);
void execute_pipe(command *command_, int *pipe_fd, int id, map<unsigned int, int*> &numbered_pipes, unsigned int line_count, string line);
void execute_redirect(command *command_, int id, map<unsigned int, int*> &numbered_pipes, unsigned int line_count, string line);
void pipe_child_in(command *command_);
void pipe_child_out(command *command_, int *pipe_fd);
void pipe_parent(command *command_);
void numbered_pipe_parent(map<unsigned int, int*> &numbered_pipes, unsigned int line_count);
void numbered_pipe_child_in(command *command_, map<unsigned int, int*> &numbered_pipes, unsigned int line_count);
void user_pipe_in(command *command_, int id);
void user_pipe_out(command *command_, int id);
void user_pipe_parent(command *command_, int id);
void wait_pid(pid_t pid);
#endif