#include "def.h"
#include "utils.h"
#include "command.h"
#include "client.h"
#include "server_utils.h"
#include "server_execute.h"

int server_sock_fd, client_sock_fd;
struct sockaddr_in server_addr, client_addr;
int nfds;
fd_set afds, rfds;
map<int, client*> client_list;
map<int, int*> user_pipes_out;
map<int, map<int, int*>> user_pipes_in;
bool **user_pipe_record;


int main(int argc, char *argv[]) {
    init_user_pipe_record();
    init_user_pipes_in();
    if (argc != 2) {
        cerr << "Usage: ./np_simple [port number]." << endl;
        return 0;
    }

    int port = atoi(argv[1]);

    int addr_len = sizeof(struct sockaddr_in);

    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(port);

    char command_line[MAX_INPUT_LEN];

    if ((server_sock_fd = socket(AF_INET , SOCK_STREAM , 0)) < 0) {
        cerr << "socket failed." << endl;
        return 0;
    }

    int option = 1;
    if (setsockopt(server_sock_fd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(int)) < 0) {
        cerr << "setsockopt failed." << endl;
        return 0;
    }

    if (bind(server_sock_fd,(struct sockaddr *)&server_addr , sizeof(server_addr)) < 0) {
        cerr << "bind failed." << endl;
        return 0;
    }

    if (listen(server_sock_fd , 30) < 0) {
        cerr << "listen failed." << endl;
        return 0;
    }

    nfds = getdtablesize();
    FD_ZERO(&afds);
    FD_SET(server_sock_fd, &afds);


    vector<string> input_argv;

    client_list = init_client_list();

    while(1) {
        memcpy(&rfds, &afds, sizeof(rfds));
        if (select(nfds, &rfds, (fd_set *)0, (fd_set *)0, (struct timeval *)0) < 0) {
            cerr << "select failed." << endl;
            if(errno == EINTR) {
                continue;
            }
            exit(0);    
        }
        if (FD_ISSET(server_sock_fd, &rfds)) { //new user
            if ((client_sock_fd = accept(server_sock_fd, (struct sockaddr *)&client_addr, (socklen_t*)&addr_len)) < 0) {
                cerr << "accept failed." << endl;
                continue;
            }
            int client_id = find_avaible_id();
            client *new_client = init_client(client_id, client_addr, client_sock_fd);
            client_list[client_id] = new_client;

            FD_SET(client_sock_fd, &afds);
            write_welcome_message(client_sock_fd);
            broadcast_login(new_client);
            write_prompt(client_sock_fd);
        }
        //old user
        int *std;
        for (int i = 1; i <= 30; ++i) {
            if (client_list[i] == NULL) {
                continue;
            }
            int fd = client_list[i]->socketfd;
            if (fd != server_sock_fd && FD_ISSET(fd, &rfds)) {
                client *client_ = client_list[i];
                memset(&command_line, '\0', MAX_INPUT_LEN);

                if((read(fd, command_line, MAX_INPUT_LEN)) <= 0) {
                    if(errno == EINTR) {
                        write_prompt(fd);
                        continue;
                    }
                    for (int j = 1; j <= 30; ++j) {
                        user_pipe_record[client_->id][i] = false;
                        user_pipe_record[i][client_->id] = false;
                    }
                    broadcast_logout(client_);
                    close(fd);
                    FD_CLR(fd, &afds);
                    client_list[i] = NULL;
                }
                else {
                    std = save_std();
                    dup2(fd, STDOUT_FILENO);
                    dup2(fd, STDERR_FILENO);
                    set_env_variable(client_);
                    string line(command_line); 
                    line = delete_special_character(line);
                    input_argv = split(line, " ");

                    if (do_server_build_in_command(input_argv, client_, line) != 0) {
                        write_prompt(fd);
                        unset_env_variable(client_);
                        restore_std(std);
                        continue;
                    }
                    vector<command*> commands = server_parse(input_argv);
                    client_->line_count = server_execute_commands(commands, client_, line);
                    delete_commands(commands);
                    write_prompt(fd);
                    unset_env_variable(client_);
                    restore_std(std);
                }
                
            }
        }
    }
    
}
