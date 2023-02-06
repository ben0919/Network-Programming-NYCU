#ifndef SERVER_UTILS_H
#define SERVER_UTILS_H
#include "def.h"
#include "client.h"
#include "command.h"

extern int server_sock_fd, client_sock_fd;
extern struct sockaddr_in server_addr, client_addr;
extern fd_set afds, rfds;
extern int nfds;
extern map<int, client*> client_list;
extern map<int, int*> user_pipes_out;
extern map<int, map<int, int*>> user_pipes_in;
extern bool **user_pipe_record;

void init_user_pipe_record();
void init_user_pipes_in();
void broadcast(string message);
client* init_client(int client_id, struct sockaddr_in client_addr, int client_sock);
map<int, client*> init_client_list();
int find_avaible_id();
void write_welcome_message(int fd);
void broadcast_login(client *new_client);
void write_prompt(int fd);
void broadcast_logout(client *client_);
void set_env_variable(client *client_);
void unset_env_variable(client *client_);
int* save_std();
void restore_std(int *std);
vector<command*> server_parse(vector<string> &input_argv);

#endif