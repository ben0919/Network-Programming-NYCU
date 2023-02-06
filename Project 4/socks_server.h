#ifndef SOCKS_SERVER_H
#define SOCKS_SERVER_H

#include <cstdlib>
#include <iostream>
#include <memory>
#include <utility>
#include <boost/asio.hpp>
#include <string>
#include <vector>
#include <unistd.h>
#include <signal.h> 
#include <bitset>
#include <fstream>

#define MAX_LENGTH 16000

using boost::asio::ip::tcp;
using namespace std;

struct request {
    int VN;
    int CD;
    int port;
    string ip;
    request(int vn = 0, int cd = 0, int p = 0, string i = ""): VN(vn), CD(cd), port(p), ip(i) {

    }
};

struct request parse_request(uint8_t *data_);
bool check_pemission(struct request request_);
#endif
