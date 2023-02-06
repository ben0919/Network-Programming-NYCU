#include "def.h"
#include "server_execute.h"


int do_server_build_in_command(vector<string> argv, client* client_, string line) {
    int fd = client_->socketfd;
    string message;
    if (argv.size() == 0) {
        return 1;
    }
    if (argv[0] == "exit") {
        broadcast_logout(client_);
        close(client_->socketfd);
        FD_CLR(client_->socketfd, &afds);
        client_list[client_->id] = NULL;
        for (int i = 0; i <=30 ; ++i) {
            user_pipe_record[client_->id][i] = false;
            user_pipe_record[i][client_->id] = false;
        }
        return 1;
    }
    if (argv[0] == "printenv") {
        if (argv.size() != 2) {
            cout << "Usage: printenv <variable name>" << endl;
            return -1;
        } else {
            char *result;
            if((result = getenv(argv[1].c_str()))) {
                cout << result << endl;
                return 1;
            } else {
                return -1;
            }
        }
    }
    if (argv[0] == "setenv") {
        if (argv.size() != 3) {
            cout << "Usage: setenv <variable name> <value to assign>" << endl;
            return -1;
        } else {
            if(setenv(argv[1].c_str(), argv[2].c_str(), 1) == 0) {
                env_variable *env = new env_variable;
                env->name = argv[1];
                env->value = argv[2];
                client_->env_list.push_back(env);
                return 1;
            } else {
                return -1;
            }
        }
    }
    if (argv[0] == "who") {
        if (argv.size() != 1) {
            cout << "Usage: who" << endl;
            return -1;
        }
        message = "<ID>\t<nickname>\t<IP:port>\t<indicate me>\n";
        for (int i = 1; i <= 30; ++i) {
            if (client_list[i] != NULL) {
                client_ = client_list[i];
                message += (to_string(i) + "\t" + client_->name + "\t" + client_->ip +  ":" + to_string(client_->port));
                if (client_->socketfd == fd) {
                    message += ("\t<-me");
                }
                message += "\n";
            }
        }
        write(fd, message.c_str(), message.length());
        return 1;
    }
    if (argv[0] == "yell") {
        if (argv.size() == 1) {
            cout << "Usage: yell <message>" << endl;
            return -1;
        }
        int index = line.find(argv[0]);
        index = index + argv[0].length();
        while(line[index] == ' ') {
            ++index;
        }
        message = ("*** " + client_->name + " yelled ***: " + line.substr(index, line.length()) + "\n");
        broadcast(message);
        return 1;
    }
    if (argv[0] == "tell") {
        if (argv.size() <= 2) {
            cout << "Usage: tell <user id> <message>" << endl;
            return -1;
        }
        int index = line.find(argv[1]);
        index = index + argv[1].length();
        while(line[index] == ' ') {
            ++index;
        }
        int target = stoi(argv[1]);
        if (client_list[target] == NULL) {
            message = "*** Error: user #" + argv[1] + " does not exist yet. ***\n";
            write(fd, message.c_str(), message.length());
            return 1;
        }
        message = "*** " + client_->name+ " told you ***: " + line.substr(index, line.length()) + "\n";
        write(client_list[target]->socketfd, message.c_str(), message.length());
        return 1;
    }
    if (argv[0] == "name") {
        if (argv.size() != 2) {
            cout << "Usage: name <new name>" << endl;
            return -1;
        }
        client *temp;
        for (int i = 1; i <= 30; ++i) {
            if (client_list[i] != NULL) {
                temp = client_list[i];
                if (argv[1] == temp->name) {
                    message = "*** User \'" + argv[1] +  "\' already exists. ***\n";
                    write(fd, message.c_str(), message.length());
                    return 1;
                }
            }
        }
        message = "*** User from " + client_->ip + ":" + to_string(client_->port) + " is named \'" + argv[1] + "\'. ***\n";
        client_->name = argv[1];
        broadcast(message);
        return 1;
    }
    return 0;
}

unsigned int server_execute_commands(vector<command*> commands, client* client_, string line){
    int N = commands.size();
    int *pipe_fd;
    int dev_null_fd;
    for (int i = 0; i < N; ++i) {
        if (commands[i]->type == PIPE){ 
            if (commands[i]->pipe_after) {
                pipe_fd = new int[2];
                pipe(pipe_fd);
                commands[i+1]->pipe_in_fd = pipe_fd;
            }
            if (commands[i]->user_pipe_in) {
                if (client_list[commands[i]->in_id] == NULL) {
                    cout << "*** Error: user #"  << commands[i]->in_id << " does not exist yet. ***" << endl;
                    pipe_fd = new int[2];
                    dev_null_fd = open("/dev/null", O_RDWR);
                    pipe_fd[0] = dev_null_fd;
                    pipe_fd[1] = dev_null_fd;
                    user_pipes_in[client_->id][commands[i]->in_id] = pipe_fd;
                } else {
                    if (!user_pipe_record[commands[i]->in_id][client_->id]) {
                        cout << "*** Error: the pipe #" << commands[i]->in_id << "->#" << client_->id << " does not exist yet. ***" << endl;
                        pipe_fd = new int[2];
                        dev_null_fd = open("/dev/null", O_RDWR);
                        pipe_fd[0] = dev_null_fd;
                        pipe_fd[1] = dev_null_fd;
                        user_pipes_in[client_->id][commands[i]->in_id] = pipe_fd;
                    } else {
                        string message = "*** " + client_->name + " (#" + to_string(client_->id) + ") just received from " + client_list[commands[i]->in_id]->name + " (#" + to_string(commands[i]->in_id) +") by \'" + line + "\' ***\n";
                        broadcast(message);
                    }
                }
            }
            if (commands[i]->user_pipe_out) {
                if (client_list[commands[i]->out_id] == NULL) {
                    cout << "*** Error: user #"  << commands[i]->out_id << " does not exist yet. ***" << endl;
                    pipe_fd = new int[2];
                    dev_null_fd = open("/dev/null", O_RDWR);
                    pipe_fd[0] = dev_null_fd;
                    pipe_fd[1] = dev_null_fd;
                    user_pipes_out[client_->id] = pipe_fd;
                } else {
                    if (!user_pipe_record[client_->id][commands[i]->out_id]) {
                        pipe_fd = new int[2];
                        pipe(pipe_fd);
                        user_pipes_out[client_->id] = pipe_fd;
                        user_pipes_in[commands[i]->out_id][client_->id] = pipe_fd;
                        user_pipe_record[client_->id][commands[i]->out_id] = true;
                        string message = "*** " + client_->name + " (#" + to_string(client_->id) + ") just piped \'" + line + "\' to " + client_list[commands[i]->out_id]->name + " (#" + to_string(commands[i]->out_id) + ") ***\n";
                        broadcast(message);
                    } else {
                        cout << "*** Error: the pipe #" << client_->id << "->#" << commands[i]->out_id <<  " already exists. ***" << endl;
                        pipe_fd = new int[2];
                        dev_null_fd = open("/dev/null", O_RDWR);
                        pipe_fd[0] = dev_null_fd;
                        pipe_fd[1] = dev_null_fd;
                        user_pipes_out[client_->id] = pipe_fd;
                    }
                }
            }
            execute_pipe(commands[i], pipe_fd, client_, line);
        } else if (commands[i]->type == NUMBERED_PIPE) {
            unsigned int target = client_->line_count + commands[i]->pipe_number;
            if (commands[i]->user_pipe_in) {
                if (client_list[commands[i]->in_id] == NULL) {
                    cout << "*** Error: user #"  << commands[i]->in_id << " does not exist yet. ***" << endl;
                    pipe_fd = new int[2];
                    dev_null_fd = open("/dev/null", O_RDWR);
                    pipe_fd[0] = dev_null_fd;
                    pipe_fd[1] = dev_null_fd;
                    user_pipes_in[client_->id][commands[i]->in_id] = pipe_fd;
                } else {
                    if (!user_pipe_record[commands[i]->in_id][client_->id]) {
                        cout << "*** Error: the pipe #" << commands[i]->in_id << "->#" << client_->id << " does not exist yet. ***" << endl;
                        pipe_fd = new int[2];
                        dev_null_fd = open("/dev/null", O_RDWR);
                        pipe_fd[0] = dev_null_fd;
                        pipe_fd[1] = dev_null_fd;
                        user_pipes_in[client_->id][commands[i]->in_id] = pipe_fd;
                    } else {
                        string message = "*** " + client_->name + " (#" + to_string(client_->id) + ") just received from " + client_list[commands[i]->in_id]->name + " (#" + to_string(commands[i]->in_id) +") by \'" + line + "\' ***\n";
                        broadcast(message);
                    }
                }
            }
            if (client_->numbered_pipes.find(target) == client_->numbered_pipes.end()) {
                pipe_fd = new int[2];
                pipe(pipe_fd);
                client_->numbered_pipes[target] = pipe_fd;
            } else {
                pipe_fd = client_->numbered_pipes[target];
            }
            execute_pipe(commands[i], pipe_fd, client_, line);
            if (i < N - 1) {
                ++client_->line_count;
            }
        } else if (commands[i]->type == REDIRECT) {
            execute_redirect(commands[i], client_, line);
        }
    }
    ++client_->line_count;
    return client_->line_count;
}

void execute_pipe(command *command_, int *pipe_fd, client* client_, string line){
    int status;
    pid_t pid = fork();
    while (pid == -1) {
        wait_pid(pid);
        pid = fork();
    }
    if (pid == 0) { //child
        user_pipe_in(command_, client_);
        user_pipe_out(command_, client_);
        numbered_pipe_child_in(command_, client_->numbered_pipes, client_->line_count);
        pipe_child_in(command_);
        pipe_child_out(command_, pipe_fd);
        if(execvp(command_->name, command_->argv) < 0) {
            cerr << "Unknown command: [" << command_->name << "]." << endl;
            exit(0);
        }
    } else { //parent
        user_pipe_parent(command_, client_);
        numbered_pipe_parent(client_->numbered_pipes, client_->line_count);
        pipe_parent(command_);
        if (!command_->pipe_after && !command_->numbered_pipe_after !command_->user_pipe_out) {
            wait_pid(-1);
        }
        
    }
}

void execute_redirect(command *command_, client* client_, string line) {
    int status;
    pid_t pid = fork();
    while (pid == -1) {
        wait_pid(pid);
        pid = fork();
    }
    if (pid == 0) { //child
        user_pipe_in(command_, client_);
        numbered_pipe_child_in(command_, client_->numbered_pipes, client_->line_count);
        pipe_child_in(command_);
        int fd = open(command_->file_name, O_WRONLY|O_CREAT|O_TRUNC, 0644);
        dup2(fd, STDOUT_FILENO);
        if(execvp(command_->name, command_->argv) < 0) {
            cerr << "Unknown command: [" << command_->name << "]." << endl;
            exit(0);
        }
    } else { //parent
        user_pipe_parent(command_, client_);
        numbered_pipe_parent(client_->numbered_pipes, client_->line_count);
        pipe_parent(command_);
        wait_pid(pid);
    }
}

void pipe_child_in(command *command_){
    if (command_->pipe_before) {
        dup2(command_->pipe_in_fd[PIPE_READ], STDIN_FILENO);
        close(command_->pipe_in_fd[PIPE_WRITE]);
        close(command_->pipe_in_fd[PIPE_READ]);
    }
}

void pipe_child_out(command *command_, int *pipe_fd){
    if (command_->pipe_after || command_->numbered_pipe_after) { 
        if (command_->pipe_stderr) {
            dup2(pipe_fd[PIPE_WRITE], STDERR_FILENO);
        }
        dup2(pipe_fd[PIPE_WRITE], STDOUT_FILENO);
        close(pipe_fd[PIPE_READ]);
        close(pipe_fd[PIPE_WRITE]);
    }
}

void pipe_parent(command *command_){
    if (command_->pipe_before) {
        close(command_->pipe_in_fd[PIPE_READ]);
        close(command_->pipe_in_fd[PIPE_WRITE]);
    }
}

void numbered_pipe_child_in(command *command_, map<unsigned int, int*> &numbered_pipes, unsigned int line_count){
    if ((command_->numbered_pipe_before) || (!command_->pipe_before && !command_->numbered_pipe_before)) {
        if (numbered_pipes.find(line_count) != numbered_pipes.end()) {
            int *pipe_fd = numbered_pipes[line_count];
            dup2(pipe_fd[PIPE_READ], STDIN_FILENO);
            close(pipe_fd[PIPE_WRITE]);
            close(pipe_fd[PIPE_READ]);
        }
    }
}

void numbered_pipe_parent(map<unsigned int, int*> &numbered_pipes, unsigned int line_count) {
    if (numbered_pipes.find(line_count) != numbered_pipes.end()) {
        int *pipe_fd = numbered_pipes[line_count];
        close(pipe_fd[PIPE_WRITE]);
        close(pipe_fd[PIPE_READ]);
        numbered_pipes.erase(line_count);
    }
}
void user_pipe_in(command *command_, client *client_){
    if (command_->user_pipe_in) {
        int *pipe_fd = user_pipes_in[client_->id][command_->in_id];
        dup2(pipe_fd[PIPE_READ], STDIN_FILENO);
        close(pipe_fd[PIPE_WRITE]);
        close(pipe_fd[PIPE_READ]);
    }
}

void user_pipe_out(command *command_, client *client_){
    if (command_->user_pipe_out) {
        int *pipe_fd = user_pipes_out[client_->id];
        dup2(pipe_fd[PIPE_WRITE], STDOUT_FILENO);
        close(pipe_fd[PIPE_READ]);
        close(pipe_fd[PIPE_WRITE]); 
        user_pipes_out.erase(client_->id);
    }
}

void user_pipe_parent(command *command_, client *client_){
    if (command_->user_pipe_in) {
        int *pipe_fd = user_pipes_in[client_->id][command_->in_id];
        close(pipe_fd[PIPE_WRITE]);
        close(pipe_fd[PIPE_READ]);
        user_pipes_in[client_->id].erase(command_->in_id);
        if (command_->in_id <=30) {
            user_pipe_record[command_->in_id][client_->id] = false;
        }
    }
}


void wait_pid(pid_t pid) {
    int status;
    int pid_;
    while (true) {
        pid_ = waitpid(pid, &status, WNOHANG);
        if (pid_ == pid) {
            break;
        }
    }
}

