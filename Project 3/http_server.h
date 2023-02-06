#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include <cstdlib>
#include <iostream>
#include <memory>
#include <utility>
#include <boost/asio.hpp>
#include <string>
#include <vector>
#include <unistd.h>
#include <signal.h> 

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

request parse_request(string raw_request);
#endif
