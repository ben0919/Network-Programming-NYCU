#ifndef CGI_SERVER_H
#define CGI_SERVER_H

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <memory>
#include <utility>
#include <boost/asio.hpp>
#include <string>
#include <vector>
#include <unistd.h>
#include <signal.h> 
#include <map>

#define BUFFER_MAX_LEN 16000

using boost::asio::ip::tcp;
using namespace std;

struct request {
    string method;
    string uri;
    string query;
    string protocol;
    string host;
    string target;
};

struct client_info {
    string host_name;
    int port;
    string file_name;
};

request parse_request(string raw_request);
vector<client_info> parse_query(string query);
string escape(string message);


#endif