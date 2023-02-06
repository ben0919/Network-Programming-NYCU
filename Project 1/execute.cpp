#include "def.h"
#include "execute.h"

int do_build_in_command(vector<string> argv) {
    if (argv.size() == 0) {
        return 1;
    }
    if (argv[0] == "exit") {
        exit(0);
    }
    if (argv[0] == "printenv") {
        if (argv.size() != 2) {
            cout << "Usage: printenv [variable_name]" << endl;
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
            cout << "Usage: setenv [variable_name] [value_to_assign]" << endl;
            return -1;
        } else {
            if(setenv(argv[1].c_str(), argv[2].c_str(), 1) == 0) {
                return 1;
            } else {
                return -1;
            }
        }
    }
    return 0;
}

void execute_commands(vector<command*> commands){
    int N = commands.size();
    queue<int*> pipes;
    int *init = new int[2]();
    init[0] = STDIN_FILENO;
    init[1] = STDOUT_FILENO;
    pipes.push(init);
    for (int i = 0; i < N; ++i) {
        if (commands[i]->type == PIPE){
            int *pipe_fd = new int[2];
            pipe(pipe_fd);
            pipes.push(pipe_fd);
            execute_pipe(commands[i], pipe_fd, pipes);
            delete[] pipes.front();
            pipes.pop();
        } else if (commands[i]->type == NUMBERED_PIPE) {
            int *pipe_fd;
            int target = line_count + commands[i]->pipe_number;
            if (numbered_pipes.find(target) == numbered_pipes.end()) {
                pipe_fd = new int[2];
                pipe(pipe_fd);
                numbered_pipes[target] = pipe_fd;
            } else {
                pipe_fd = numbered_pipes[target];
            }
            execute_pipe(commands[i], pipe_fd, pipes);
            if (i < N - 1) {
                ++line_count;
            }
        } else if (commands[i]->type == REDIRECT) {
            execute_redirect(commands[i], pipes);
        }
    }
    while(!pipes.empty()) {
        delete[] pipes.front();
        pipes.pop();
    }
    ++line_count;
}

void execute_pipe(command *command_, int *pipe_fd, queue<int*> &pipes){
    int status;
    pid_t pid = fork();
    while (pid == -1) {
        wait_pid(pid);
        pid = fork();
    }
    if (pid == 0) { //child
        numbered_pipe_child_in(command_);
        pipe_child_in(command_, pipes);
        pipe_child_out(command_, pipe_fd);
        if(execvp(command_->name, command_->argv) < 0) {
            cerr << "Unknown command: [" << command_->name << "]." << endl;
            exit(0);
        }
    } else { //parent
        numbered_pipe_parent();
        pipe_parent(command_, pipes);
        if (!command_->pipe_after && !command_->numbered_pipe_after) {
            wait_pid(pid);
        }
    }
}

void execute_redirect(command *command_, queue<int*> &pipes) {
    int status;
    pid_t pid = fork();
    while (pid == -1) {
        wait_pid(pid);
        pid = fork();
    }
    if (pid == 0) { //child
        numbered_pipe_child_in(command_);
        pipe_child_in(command_, pipes);
        int fd = open(command_->file_name, O_WRONLY|O_CREAT|O_TRUNC, 0644);
        dup2(fd, STDOUT_FILENO);
        if(execvp(command_->name, command_->argv) < 0) {
            cerr << "Unknown command: [" << command_->name << "]." << endl;
            exit(0);
        }
    } else { //parent
        numbered_pipe_parent();
        pipe_parent(command_, pipes);
        wait_pid(pid);
    }
}

void pipe_child_in(command *command_, queue<int*> &pipes){
    if (command_->pipe_before) {
        close(pipes.front()[PIPE_WRITE]);
        dup2(pipes.front()[PIPE_READ], STDIN_FILENO);
        close(pipes.front()[PIPE_READ]);
    }
}

void pipe_child_out(command *command_, int *pipe_fd){
    if (command_->pipe_after || command_->numbered_pipe_after) {
        if (command_->pipe_stderr) {
            dup2(pipe_fd[PIPE_WRITE], STDERR_FILENO);
        }
        close(pipe_fd[PIPE_READ]);
        dup2(pipe_fd[PIPE_WRITE], STDOUT_FILENO);
        close(pipe_fd[PIPE_WRITE]);
    }
}

void pipe_parent(command *command_, queue<int*> &pipes){
    if (command_->pipe_before) {
        close(pipes.front()[PIPE_READ]);
        close(pipes.front()[PIPE_WRITE]);
    }
}

void numbered_pipe_child_in(command *command_){
    if ((command_->numbered_pipe_before) || (!command_->pipe_before && !command_->numbered_pipe_before)) {
        if (numbered_pipes.find(line_count) != numbered_pipes.end()) {
            int *pipe_fd = numbered_pipes[line_count];
            close(pipe_fd[PIPE_WRITE]);
            dup2(pipe_fd[PIPE_READ], STDIN_FILENO);
            close(pipe_fd[PIPE_READ]);
        }
    }
}

void numbered_pipe_parent() {
    if (numbered_pipes.find(line_count) != numbered_pipes.end()) {
        int *pipe_fd = numbered_pipes[line_count];
        close(pipe_fd[PIPE_WRITE]);
        close(pipe_fd[PIPE_READ]);
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