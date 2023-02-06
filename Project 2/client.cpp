#include "def.h"
#include "client.h"

env_variable::env_variable() {
    this->name = "";
    this->value = "";
};

client::client() {
    this->id = -1;
    this->name = "";
    this->ip = "";
    this->port = -1;
    this->socketfd = -1;
    this->line_count = 0;
};