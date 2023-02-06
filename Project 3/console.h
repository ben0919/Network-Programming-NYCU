#ifndef CONSOLE_H
#define CONSOLE_H

#include <cstdlib>
#include <iostream>
#include <fstream>
#include <memory>
#include <utility>
#include <boost/asio.hpp>
#include <string>
#include <vector>
#include <unistd.h>
#include <signal.h> 

#define BUFFER_MAX_LEN 16000

using boost::asio::ip::tcp;
using namespace std;

struct client_info {
    string host_name;
    int port;
    string file_name;
};

vector<client_info> parse_query(string query);
void print_body(vector<client_info> clients);
string escape(string message);

#endif