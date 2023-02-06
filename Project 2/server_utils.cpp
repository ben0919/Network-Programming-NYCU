#include "def.h"
#include "server_utils.h"

void init_user_pipe_record() {
    user_pipe_record = new bool*[31];
    for (int i = 0; i <= 30; ++i) {
        user_pipe_record[i] = new bool[31];
        for (int j = 0; j < 31; ++j) {
            user_pipe_record[i][j] = false;
        }
    }
}

void init_user_pipes_in() {
    for (int i = 1; i <= 30 ; ++i) {
        map<int, int*> temp;
        user_pipes_in[i] = temp;
    }
}

void broadcast(string message) {
    for (int i = 1; i <= 30; ++i) {
        if (client_list[i] == NULL) {
            continue;
        }
        int fd = client_list[i]->socketfd;
        if (fd != server_sock_fd && FD_ISSET(fd, &afds)) {
            write(fd, message.c_str(), message.length());
        }
    }
}

client* init_client(int client_id, struct sockaddr_in client_addr, int client_sock_fd) {
    client *new_client = new client;
    new_client->id = client_id;
    new_client->name = "(no name)";
    new_client->ip = inet_ntoa(client_addr.sin_addr);
    new_client->port = ntohs(client_addr.sin_port);
    new_client->socketfd = client_sock_fd;
    
    env_variable *env = new env_variable;
    env->name = "PATH";
    env->value = "bin:.";
    new_client->env_list.push_back(env);
    return new_client;
}

map<int, client*> init_client_list() {
    map<int, client*> list;
    for (int i = 1; i <= 30; ++i) {
        list[i] = NULL;
    }
    return list;
}

int find_avaible_id() {
    for (int i = 1; i <= 30; ++i) {
        if (client_list[i] == NULL) {
            return i;
        }
    }
    return 0;
}

void write_welcome_message(int fd) {
    string message = "****************************************\n** Welcome to the information server. **\n****************************************\n";
    write(client_sock_fd, message.c_str(), message.length());
}

void broadcast_login(client *new_client) {
    string message =  "*** User \'" + new_client->name + "\' entered from " + new_client->ip + ":" + to_string(new_client->port) + ". ***\n";
    broadcast(message);
}

void write_prompt(int fd){
    string message = "% ";
    write(fd, message.c_str(), message.length());
}

void broadcast_logout(client *client_) {
    string message =  "*** User \'" + client_->name + "\' left. ***\n";
    broadcast(message);
}

void set_env_variable(client *client_) {
    for (int i = 0; i < client_->env_list.size(); ++i) {
        setenv(client_->env_list[i]->name.c_str(), client_->env_list[i]->value.c_str(), 1);
    }
}

void unset_env_variable(client *client_) {
    for (int i = 0; i < client_->env_list.size(); ++i) {
        unsetenv(client_->env_list[i]->name.c_str());
    }
}


int* save_std() {
    int *std = new int[3];
    std[0] = dup(STDIN_FILENO);
    std[1] = dup(STDOUT_FILENO);
    std[2] = dup(STDERR_FILENO);
    return std;
}

void restore_std(int *std) {
    dup2(std[0], STDIN_FILENO);
    dup2(std[1], STDOUT_FILENO);
    dup2(std[2], STDERR_FILENO);
    close(std[0]);
    close(std[1]);
    close(std[2]);
    delete[] std;
}

vector<command*> server_parse(vector<string> &input_argv) {
    vector<command*> commands;
    vector<string> argv;
    bool pipe_before = false;
    bool numbered_pipe_before = false;
    int in_id = 0;
    int out_id = 0;
    bool user_pipe_in = false;
    int N = input_argv.size();
    for(int i = 0; i < N; ++i) {
        if (input_argv[i].find("|") != string::npos) {
            if (input_argv[i] == "|") {
                command *temp = new command(PIPE, argv, pipe_before, true, numbered_pipe_before, false, false);
                if (user_pipe_in) {
                    temp->in_id = in_id;
                    temp->user_pipe_in = user_pipe_in;
                }
                commands.push_back(temp);
                pipe_before = true;
                numbered_pipe_before = false;
                user_pipe_in = false;
                argv.clear();
            } else {
                unsigned int pipe_number = stoi(input_argv[i].substr(1));
                command *temp = new command(NUMBERED_PIPE, argv, pipe_number, pipe_before, false, numbered_pipe_before, true, false);
                if (user_pipe_in) {
                    temp->in_id = in_id;
                    temp->user_pipe_in = user_pipe_in;
                }
                commands.push_back(temp);
                pipe_before = false;
                numbered_pipe_before = true;
                user_pipe_in = false;
                argv.clear();
            }
        } else if (input_argv[i].find("!") != string::npos) {
            unsigned int pipe_number = stoi(input_argv[i].substr(1));
            command *temp = new command(NUMBERED_PIPE, argv, pipe_number, pipe_before, false, numbered_pipe_before, true, true);
            if (user_pipe_in) {
                temp->in_id = in_id;
                temp->user_pipe_in = user_pipe_in;
            }
            commands.push_back(temp);
            pipe_before = false;
            numbered_pipe_before = true;
            user_pipe_in = false;
            argv.clear();
        } else if (input_argv[i].find(">") != string::npos) {
            if (input_argv[i] == ">") {
                if (i == input_argv.size() - 1) {
                    cout << "Usage: [command] > [file_name] " << endl;
                    break;
                } else {
                    command *temp = new command(REDIRECT, argv, input_argv[i+1], pipe_before, false, numbered_pipe_before, false, false);
                    if (user_pipe_in) {
                        temp->in_id = in_id;
                        temp->user_pipe_in = user_pipe_in;
                    }
                    commands.push_back(temp);
                    break;
                }
            } else {
                out_id = stoi(input_argv[i].substr(1));
                command *temp = new command(PIPE, argv, pipe_before, false, numbered_pipe_before, false, false);
                if (i < N-1) {
                    if (input_argv[i+1].find("<") != string::npos) {
                        in_id = stoi(input_argv[i+1].substr(1));
                        temp->in_id = in_id;
                        temp->user_pipe_in = true;
                        ++i;
                    }
                }
                if (user_pipe_in) {
                    temp->in_id = in_id;
                    temp->user_pipe_in = user_pipe_in;
                }
                temp->user_pipe_out = true;
                temp->out_id = out_id;
                commands.push_back(temp);
                pipe_before = false;
                numbered_pipe_before = false;
                user_pipe_in = false;
                argv.clear();
            }
                
        } else if (input_argv[i].find("<") != string::npos) {
            in_id = stoi(input_argv[i].substr(1));
            user_pipe_in = true;
            if (i == N-1) {
                command *temp = new command(PIPE, argv, pipe_before, false, numbered_pipe_before, false, false);
                temp->user_pipe_in = true;
                temp->in_id = in_id;
                commands.push_back(temp);
            }

        } else if (i == N-1) {
            argv.push_back(input_argv[i]);
            command *temp = new command(PIPE, argv, pipe_before, false, numbered_pipe_before, false, false);
            commands.push_back(temp);
        } else {
            argv.push_back(input_argv[i]);
        }
        
    }
    return commands;
}