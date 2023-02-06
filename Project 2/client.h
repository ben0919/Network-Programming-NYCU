#ifndef CLIENT_H
#define CLIENT_H

#include "def.h"

class env_variable {
public:
    env_variable();
    string name;
    string value;
};

class client {
public:
    client();
    int id;
    string name;
    string ip;
    uint16_t port;
    int socketfd;
    vector<env_variable*> env_list;
    map<unsigned int, int*> numbered_pipes;
    unsigned int line_count;
};


#endif