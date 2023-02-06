#include "def.h"
#include "execute.h"

using namespace std;

int do_build_in_command(vector<string> argv) {
    if (argv.size() == 0) {
        return 1;
    }
    if (argv[0] == "exit") {
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
    return 0;
}

unsigned int execute_commands(vector<command*> commands, map<unsigned int, int*> &numbered_pipes, unsigned int line_count){
    int N = commands.size();
    int *pipe_fd;
    for (int i = 0; i < N; ++i) {
        if (commands[i]->type == PIPE){ 
            if (commands[i]->pipe_after) {
                pipe_fd = new int[2];
                pipe(pipe_fd);
                commands[i+1]->pipe_in_fd = pipe_fd;
            }
            execute_pipe(commands[i], pipe_fd, numbered_pipes, line_count);
        } else if (commands[i]->type == NUMBERED_PIPE) {
            unsigned int target = line_count + commands[i]->pipe_number;
            if (numbered_pipes.find(target) == numbered_pipes.end()) {
                pipe_fd = new int[2];
                pipe(pipe_fd);
                numbered_pipes[target] = pipe_fd;
            } else {
                pipe_fd = numbered_pipes[target];
            }
            execute_pipe(commands[i], pipe_fd, numbered_pipes, line_count);
            if (i < N - 1) {
                ++line_count;
            }
        } else if (commands[i]->type == REDIRECT) {
            execute_redirect(commands[i], numbered_pipes, line_count);
        }
    }
    ++line_count;
    return line_count;
}

void execute_pipe(command *command_, int *pipe_fd, map<unsigned int, int*> &numbered_pipes, unsigned int line_count){
    int status;
    pid_t pid = fork();
    while (pid == -1) {
        wait_pid(pid);
        pid = fork();
    }
    if (pid == 0) { //child
        numbered_pipe_child_in(command_, numbered_pipes, line_count);
        pipe_child_in(command_);
        pipe_child_out(command_, pipe_fd);
        if(execvp(command_->name, command_->argv) < 0) {
            cerr << "Unknown command: [" << command_->name << "]." << endl;
            exit(0);
        }
    } else { //parent
        numbered_pipe_parent(numbered_pipes, line_count);
        pipe_parent(command_);
        if (!command_->pipe_after && !command_->numbered_pipe_after) {
            wait_pid(-1);
        }
    }
}

void execute_redirect(command *command_, map<unsigned int, int*> &numbered_pipes, unsigned int line_count) {
    int status;
    pid_t pid = fork();
    while (pid == -1) {
        wait_pid(pid);
        pid = fork();
    }
    if (pid == 0) { //child
        numbered_pipe_child_in(command_, numbered_pipes, line_count);
        pipe_child_in(command_);
        int fd = open(command_->file_name, O_WRONLY|O_CREAT|O_TRUNC, 0644);
        dup2(fd, STDOUT_FILENO);
        if(execvp(command_->name, command_->argv) < 0) {
            cerr << "Unknown command: [" << command_->name << "]." << endl;
            exit(0);
        }
    } else { //parent
        numbered_pipe_parent(numbered_pipes, line_count);
        pipe_parent(command_);
        wait_pid(-1);
    }
}

void pipe_child_in(command *command_){
    if (command_->pipe_before) {
        close(command_->pipe_in_fd[PIPE_WRITE]);
        dup2(command_->pipe_in_fd[PIPE_READ], STDIN_FILENO);
        close(command_->pipe_in_fd[PIPE_READ]);
    }
}

void pipe_child_out(command *command_, int* pipe_fd){
    if (command_->pipe_after || command_->numbered_pipe_after) { 
        if (command_->pipe_stderr) {
            dup2(pipe_fd[PIPE_WRITE], STDERR_FILENO);
        }
        close(pipe_fd[PIPE_READ]);
        dup2(pipe_fd[PIPE_WRITE], STDOUT_FILENO);
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
            close(pipe_fd[PIPE_WRITE]);
            dup2(pipe_fd[PIPE_READ], STDIN_FILENO);
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