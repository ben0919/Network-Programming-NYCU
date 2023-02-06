#include "np_multi_proc.h"

struct shared_memory *shmptr;
int shmid;
int current_sock;
int user_pipe_in_fd[31];

int find_available_id() {
    sem_wait(&(shmptr->sem));
    for (int i = 1; i <= 30; ++i) {
        if (shmptr->available_id[i] == true) {
            sem_post(&(shmptr->sem));
            return i;
        }
    }
    sem_post(&(shmptr->sem));
    return 0;
}

void init_shared_momory() {
    for (int i = 1; i <= 30; ++i) {
        shmptr->available_id[i] = true;
    }
    shmptr->user_num = 0;
    while (sem_init(&(shmptr->sem), 1, 1) < 0) {
        cerr << "sem_init failed." << endl;
    }
    for (int i = 1; i <= 30; ++i) {
        for (int j = 1; j <= 30; ++j) {
            shmptr->user_pipe_record[i][j] = false;
        }
    }
}

void parentINT(int signo) {
	shmdt((void*)shmptr);
	shmctl(shmid, IPC_RMID, NULL);
	exit(0);
}

void childINT(int signo) {
    shmdt((void*)shmptr);
	exit(0);
}

void recvUSR1(int signo) { // handle broadcast messages
	string message = shmptr->message;
	write(current_sock, message.c_str(), message.size());
}

void recvUSR2(int signo) { // handle user pipes
    string file_name = "user_pipe/" + to_string(shmptr->user_pipe_from) + "to" + to_string(shmptr->user_pipe_to);
    user_pipe_in_fd[shmptr->user_pipe_from] = open(file_name.c_str(), O_RDONLY);
}

void write_welcome_message(int fd) {
    string message = "****************************************\n** Welcome to the information server. **\n****************************************\n";
    write(fd, message.c_str(), message.length());
}

void broadcast(string message) {
    strcpy(shmptr->message, message.c_str());
    for(int i = 1; i <= 30; ++i) {
        if(shmptr->available_id[i] == false) {
            kill(shmptr->client_list[i].pid, SIGUSR1);
        }
    }
}

void broadcast_login(int id) {
    string message =  "*** User \'" + string(shmptr->client_list[id].name) + "\' entered from " + string(shmptr->client_list[id].ip) + ":" + to_string(shmptr->client_list[id].port) + ". ***\n";
    broadcast(message);
}

void broadcast_logout(int id) {
    string message = "*** User \'" + string(shmptr->client_list[id].name) + "\' left. ***\n";
    broadcast(message);
}

void write_prompt(int fd){
    string message = "% ";
    write(fd, message.c_str(), message.length());
}

int do_server_build_in_command(vector<string> argv, int id, string line) {
    string message;
    if (argv.size() == 0) {
        return 1;
    }
    if (argv[0] == "exit") {
        sem_wait(&(shmptr->sem));
        shmptr->available_id[id] = true;
        close(shmptr->client_list[id].socketfd);
        for (int i = 1; i <= 30; ++i) {
            shmptr->user_pipe_record[id][i] = false;
            shmptr->user_pipe_record[i][id] = false;
        }
        broadcast_logout(id);
        sem_post(&(shmptr->sem));
        exit(0);
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
        message = "<ID>\t<nickname>\t<IP:port>\t<indicate me>";
        sem_wait(&(shmptr->sem));
        for (int i = 1; i <= 30; ++i) {
            if (shmptr->available_id[i] == false) {
                message += "\n";
                message += (to_string(i) + "\t" + string(shmptr->client_list[i].name) + "\t" + string(shmptr->client_list[i].ip) +  ":" + to_string(shmptr->client_list[i].port));
                if (shmptr->client_list[i].id == id) {
                    message += ("\t<-me");
                }
            }
        }
        sem_post(&(shmptr->sem));
        cout << message << endl;
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
        sem_wait(&(shmptr->sem));
        message = ("*** " + string(shmptr->client_list[id].name) + " yelled ***: " + line.substr(index, line.length()) + "\n");
        broadcast(message);
        sem_post(&(shmptr->sem));
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
        sem_wait(&(shmptr->sem));
        if (shmptr->available_id[target] == true) {
            cout << "*** Error: user #" + argv[1] + " does not exist yet. ***" << endl;
            sem_post(&(shmptr->sem));
            return 1;
        }
        message = "*** " + string(shmptr->client_list[id].name) + " told you ***: " + line.substr(index, line.length()) + "\n";
        strcpy(shmptr->message, message.c_str());
        kill(shmptr->client_list[target].pid, SIGUSR1);
        sem_post(&(shmptr->sem));
        return 1;
    }
    if (argv[0] == "name") {
        if (argv.size() != 2) {
            cout << "Usage: name <new name>" << endl;
            return -1;
        }
        sem_wait(&(shmptr->sem));
        for (int i = 1; i <= 30; ++i) {
            if (shmptr->available_id[i] == false) {
                if (strcmp(argv[1].c_str(), shmptr->client_list[i].name) == 0) {
                    cout << "*** User \'" + argv[1] +  "\' already exists. ***" << endl;
                    sem_post(&(shmptr->sem));
                    return 1;
                }
            }
        }
        strcpy(shmptr->client_list[id].name, argv[1].c_str());
        message = "*** User from " + string(shmptr->client_list[id].ip) + ":" + to_string(shmptr->client_list[id].port) + " is named \'" + argv[1] + "\'. ***\n";
        broadcast(message);
        sem_post(&(shmptr->sem));
        return 1;
    }
    return 0;
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

unsigned int execute_commands(vector<command*> commands, int id, map<unsigned int, int*> &numbered_pipes, unsigned int line_count, string line){
    int N = commands.size();
    int *pipe_fd;
    int dev_null_fd;
    string file_name;
    for (int i = 0; i < N; ++i) {
        if (commands[i]->type == PIPE){
            if (commands[i]->pipe_after) {
                pipe_fd = new int[2];
                pipe(pipe_fd);
                commands[i+1]->pipe_in_fd = pipe_fd;
            }
            if (commands[i]->user_pipe_in) {
                sem_wait(&(shmptr->sem));
                if (commands[i]->in_id > 30) {
                    cout << "*** Error: user #"  << commands[i]->in_id << " does not exist yet. ***" << endl;
                    shmptr->user_pipe_in_id[id][commands[i]->in_id] = 0;
                    sem_post(&(shmptr->sem));
                } else if (shmptr->available_id[commands[i]->in_id] == true) {
                    cout << "*** Error: user #"  << commands[i]->in_id << " does not exist yet. ***" << endl;
                    shmptr->user_pipe_in_id[id][commands[i]->in_id] = 0;
                    sem_post(&(shmptr->sem));
                } else {
                    if (!shmptr->user_pipe_record[commands[i]->in_id][id]) {
                        cout << "*** Error: the pipe #" << commands[i]->in_id << "->#" << id << " does not exist yet. ***" << endl;
                        shmptr->user_pipe_in_id[id][commands[i]->in_id] = 0;
                        sem_post(&(shmptr->sem));
                    } else {
                        string message = "*** " + string(shmptr->client_list[id].name) + " (#" + to_string(id) + ") just received from " + string(shmptr->client_list[commands[i]->in_id].name) + " (#" + to_string(commands[i]->in_id) +") by \'" + line + "\' ***\n";
                        broadcast(message);
                        sem_post(&(shmptr->sem));
                    }
                }
            }
            if (commands[i]->user_pipe_out) {
                sem_wait(&(shmptr->sem));
                if (commands[i]->out_id > 30) {
                    cout << "*** Error: user #"  << commands[i]->out_id << " does not exist yet. ***" << endl;
                    shmptr->user_pipe_out_id[id] = 0;
                    sem_post(&(shmptr->sem));
                } else if (shmptr->available_id[commands[i]->out_id] == true) {
                    cout << "*** Error: user #"  << commands[i]->out_id << " does not exist yet. ***" << endl;
                    shmptr->user_pipe_out_id[id] = 0;
                    sem_post(&(shmptr->sem));
                } else {
                    if (!shmptr->user_pipe_record[id][commands[i]->out_id]) {
                        file_name = "user_pipe/" + to_string(id) + "to" + to_string(commands[i]->out_id);
                        if (mknod(file_name.c_str(), S_IFIFO | 0666, 0) < 0) {
                            if (errno != EEXIST) {
                                cerr << "FIFO creation failed" << endl;
                            }
                        }
                        shmptr->user_pipe_out_id[id] = commands[i]->out_id;
                        shmptr->user_pipe_in_id[commands[i]->out_id][id] = id;
                        shmptr->user_pipe_from = id;
                        shmptr->user_pipe_to = commands[i]->out_id;
                        kill(shmptr->client_list[commands[i]->out_id].pid, SIGUSR2);
                        shmptr->user_pipe_record[id][commands[i]->out_id] = true;
                        string message = "*** " + string(shmptr->client_list[id].name) + " (#" + to_string(id) + ") just piped \'" + line + "\' to " + string(shmptr->client_list[commands[i]->out_id].name) + " (#" + to_string(commands[i]->out_id) + ") ***\n";
                        broadcast(message);
                        sem_post(&(shmptr->sem));
                    } else {
                        cout << "*** Error: the pipe #" << id << "->#" << commands[i]->out_id <<  " already exists. ***" << endl;
                        shmptr->user_pipe_out_id[id] = 0;
                        sem_post(&(shmptr->sem));
                    }
                }
            }
            execute_pipe(commands[i], pipe_fd, id, numbered_pipes, line_count, line);
        } else if (commands[i]->type == NUMBERED_PIPE) {
            unsigned int target = line_count + commands[i]->pipe_number;
            if (commands[i]->user_pipe_in) {
                sem_wait(&(shmptr->sem));
                if (commands[i]->in_id > 30) {
                    cout << "*** Error: user #"  << commands[i]->in_id << " does not exist yet. ***" << endl;
                    shmptr->user_pipe_in_id[id][commands[i]->in_id] = 0;
                    sem_post(&(shmptr->sem));
                } else if (shmptr->available_id[commands[i]->in_id] == true) {
                    cout << "*** Error: user #"  << commands[i]->in_id << " does not exist yet. ***" << endl;
                    shmptr->user_pipe_in_id[id][commands[i]->in_id] = 0;
                    sem_post(&(shmptr->sem));
                } else {
                    if (!shmptr->user_pipe_record[commands[i]->in_id][id]) {
                        cout << "*** Error: the pipe #" << commands[i]->in_id << "->#" << id << " does not exist yet. ***" << endl;
                        shmptr->user_pipe_in_id[id][commands[i]->in_id] = 0;
                        sem_post(&(shmptr->sem));
                    } else {
                        string message = "*** " + string(shmptr->client_list[id].name) + " (#" + to_string(id) + ") just received from " + string(shmptr->client_list[commands[i]->in_id].name) + " (#" + to_string(commands[i]->in_id) +") by \'" + line + "\' ***\n";
                        broadcast(message);
                        sem_post(&(shmptr->sem));
                    }
                }
            }
            if (numbered_pipes.find(target) == numbered_pipes.end()) {
                pipe_fd = new int[2];
                pipe(pipe_fd);
                numbered_pipes[target] = pipe_fd;
            } else {
                pipe_fd = numbered_pipes[target];
            }
            execute_pipe(commands[i], pipe_fd, id, numbered_pipes, line_count, line);
            if (i < N - 1) {
                ++line_count;
            }
        } else if (commands[i]->type == REDIRECT) {
            execute_redirect(commands[i], id, numbered_pipes, line_count, line);
        }
    }
    ++line_count;
    return line_count;
}

void execute_pipe(command *command_, int *pipe_fd, int id, map<unsigned int, int*> &numbered_pipes, unsigned int line_count, string line){
    int status;
    pid_t pid = fork();
    while (pid == -1) {
        wait_pid(pid);
        pid = fork();
    }
    if (pid == 0) { //child
        sem_wait(&(shmptr->sem));
        user_pipe_in(command_, id);
        user_pipe_out(command_, id);
        sem_post(&(shmptr->sem));
        numbered_pipe_child_in(command_, numbered_pipes, line_count);
        pipe_child_in(command_);
        pipe_child_out(command_, pipe_fd);
        if(execvp(command_->name, command_->argv) < 0) {
            cerr << "Unknown command: [" << command_->name << "]." << endl;
            exit(0);
        }
    } else { //parent
        sem_wait(&(shmptr->sem));
        user_pipe_parent(command_, id);
        sem_post(&(shmptr->sem));
        numbered_pipe_parent(numbered_pipes, line_count);
        pipe_parent(command_);
        if (!command_->pipe_after && !command_->numbered_pipe_after && !command_->user_pipe_out) {
            wait_pid(-1);
        }
        
    }
}

void execute_redirect(command *command_, int id, map<unsigned int, int*> &numbered_pipes, unsigned int line_count, string line) {
    int status;
    pid_t pid = fork();
    while (pid == -1) {
        wait_pid(pid);
        pid = fork();
    }
    if (pid == 0) { //child
        sem_wait(&(shmptr->sem));
        user_pipe_in(command_, id);
        sem_post(&(shmptr->sem));
        numbered_pipe_child_in(command_, numbered_pipes, line_count);
        pipe_child_in(command_);
        int fd = open(command_->file_name, O_WRONLY|O_CREAT|O_TRUNC, 0644);
        dup2(fd, STDOUT_FILENO);
        if(execvp(command_->name, command_->argv) < 0) {
            cerr << "Unknown command: [" << command_->name << "]." << endl;
            exit(0);
        }
    } else { //parent
        sem_wait(&(shmptr->sem));
        user_pipe_parent(command_, id);
        sem_post(&(shmptr->sem));
        numbered_pipe_parent(numbered_pipes, line_count);
        pipe_parent(command_);
        wait_pid(-1);
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

void user_pipe_in(command *command_, int id){
    if (command_->user_pipe_in) {
        if (shmptr->user_pipe_in_id[id][command_->in_id] == 0) {
            int dev_null_fd = open("/dev/null", O_RDONLY);
            dup2(dev_null_fd, STDIN_FILENO);
            close(dev_null_fd);
        } else {
            dup2(user_pipe_in_fd[command_->in_id], STDIN_FILENO);
            close(user_pipe_in_fd[command_->in_id]);
        }
    }
}

void user_pipe_out(command *command_, int id){
    if (command_->user_pipe_out) {
        if (shmptr->user_pipe_out_id[id] == 0) {
            int dev_null_fd = open("/dev/null", O_WRONLY);
            dup2(dev_null_fd, STDOUT_FILENO);
            close(dev_null_fd);
        } else {
            string file_name = "user_pipe/" + to_string(id) + "to" + to_string(shmptr->user_pipe_out_id[id]);
            int fd = open(file_name.c_str(), O_WRONLY);
            dup2(fd, STDOUT_FILENO);
            close(fd);
        }
    }
}

void user_pipe_parent(command *command_, int id){
    if (command_->user_pipe_in) {
        if (shmptr->user_pipe_in_id[id][command_->in_id] != 0) {
            close(user_pipe_in_fd[command_->in_id]);
            shmptr->user_pipe_record[command_->in_id][id] = false;
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




int main(int argc, char *argv[]) {
    if (argc != 2) {
        cerr << "Usage: ./np_simple [port number]." << endl;
        return 0;
    }
    int port = atoi(argv[1]);

    int master_sock, slave_sock; 
    
    sockaddr_in server_addr, client_addr;
    int addr_len = sizeof(struct sockaddr_in);
    

    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(port);

    char command_line[MAX_INPUT_LEN];

    if ((master_sock = socket(AF_INET , SOCK_STREAM , 0)) < 0) {
        cerr << "socket failed." << endl;
        return 0;
    }

    int option = 1;
    if (setsockopt(master_sock, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(int)) < 0) {
        cerr << "setsockopt failed." << endl;
        return 0;
    }

    if (bind(master_sock,(struct sockaddr *)&server_addr , sizeof(server_addr)) < 0) {
        cerr << "bind failed." << endl;
        return 0;
    }

    if (listen(master_sock , 30) < 0) {
        cerr << "listen failed." << endl;
        return 0;
    }

    if ((shmid = shmget(IPC_PRIVATE, sizeof(shared_memory), 0666|IPC_CREAT)) < 0 ) {
        cerr << "shmget failed." << errno << endl;
        return 0;
    }

    if ((shmptr = (struct shared_memory *) shmat(shmid, NULL, 0)) == nullptr) {
        cerr << "shmat failed."  << errno << endl;
        return 0;
    }

    init_shared_momory();
    
    signal(SIGINT, parentINT);
    signal(SIGUSR1, recvUSR1);
    signal(SIGUSR2, recvUSR2);
    signal(SIGCHLD, SIG_IGN);
    while (true) {
        int pid;
        if ((slave_sock = accept(master_sock, (struct sockaddr*)&client_addr, (socklen_t*)&addr_len)) < 0) {
            cerr << "accept failed." << endl;
            continue;
        }
        int client_id = find_available_id();
        if (client_id == 0) {
            close(slave_sock);
            continue;
        }
        current_sock = slave_sock;
        char ip[1024];
		inet_ntop(AF_INET, &client_addr.sin_addr, ip, 1024);
		int port = ntohs(client_addr.sin_port);
        char buffer[MAX_INPUT_LEN];
        string line;
        vector<string> input_argv;
        map<unsigned int, int*> numbered_pipes;
        unsigned int line_count = 0;
        while ((pid = fork()) < 0);
        if (pid == 0) {
			signal(SIGINT, childINT);
            env_variables_init();
            dup2(current_sock, STDOUT_FILENO);
            dup2(current_sock, STDERR_FILENO);

            write_welcome_message(current_sock);
            string message = "*** User \'" + string("(no name)") + "\' entered from " + string(ip) + ":" + to_string(port) + ". ***\n";	
			write(current_sock, message.c_str(), message.size());

            close(master_sock);

            while(true) {
                write_prompt(slave_sock);
                memset(buffer, 0, sizeof(buffer));
                if ((read(slave_sock, buffer, sizeof(buffer))) <= 0 ) {
                    if(errno == EINTR) {
                        write_prompt(slave_sock);
                        continue;
                    }
                    close(slave_sock);
                    sem_wait(&(shmptr->sem));
                    shmptr->available_id[client_id] = true;
                    for (int i = 1; i <= 30; ++i) {
                        shmptr->user_pipe_record[client_id][i] = false;
                        shmptr->user_pipe_record[i][client_id] = false;
                    }
                    broadcast_logout(client_id);
                    sem_post(&(shmptr->sem));
                    exit(0);
                }
                line = string(buffer);
                line = delete_special_character(line);
                input_argv = split(line, " ");
                if (do_server_build_in_command(input_argv, client_id, line) != 0) {
                    continue;
                }
                vector<command*> commands = server_parse(input_argv);
                line_count = execute_commands(commands, client_id, numbered_pipes, line_count, line);
                delete_commands(commands);
            }

        } else {
            close(slave_sock);
            string temp = "(no name)";
            sem_wait(&(shmptr->sem));
            shmptr->client_list[client_id] = client(pid, client_id, port, slave_sock, temp.c_str(), ip);
            broadcast_login(client_id);
            shmptr->available_id[client_id] = false;
            sem_post(&(shmptr->sem));
        }
    }
}