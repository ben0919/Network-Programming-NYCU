#include "def.h"
#include "utils.h"
#include "execute.h"
#include "command.h"
#include "npshell.h"

int main(int argc, char *argv[]) {
    if (argc != 2) {
        cerr << "Usage: ./np_simple [port number]." << endl;
        return 0;
    }
    int port = atoi(argv[1]);
    int server_sock, client_sock;

    if ((server_sock = socket(PF_INET , SOCK_STREAM , 0)) < 0) {
        cerr << "socket failed." << endl;
        return 0;
    }

    struct sockaddr_in server_addr, client_addr;
    int addr_len = sizeof(struct sockaddr_in);
    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(port);
    
    int option = 1;
    if (setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(int)) < 0) {
        cerr << "setsockopt failed." << endl;
        return 0;
    }

    if (bind(server_sock,(struct sockaddr *)&server_addr , sizeof(server_addr)) < 0) {
        cerr << "bind failed." << endl;
        return 0;
    }

    if (listen(server_sock , 1) < 0) {
        cerr << "listen failed." << endl;
        return 0;
    }

    while (1){
        client_sock = accept(server_sock, (struct sockaddr *)&client_addr, (socklen_t*)&addr_len);
        int pid = fork();
        if (pid == 0) {
            dup2(client_sock, STDIN_FILENO);
            dup2(client_sock, STDOUT_FILENO);
            dup2(client_sock, STDERR_FILENO);
            np_shell();
            close(client_sock);
            exit(0);
        } else {
            close(client_sock);
            wait_pid(pid);
        }
        
    }
}