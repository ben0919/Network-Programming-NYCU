#include "http_server.h"

class session: public std::enable_shared_from_this<session> {
public:
    session(tcp::socket socket): socket_(std::move(socket)) {

    }
    void start() {
        do_read();
    }
private:
    void do_read() {
        auto self(shared_from_this());
        socket_.async_read_some(boost::asio::buffer(data_, max_length), 
            [this, self](boost::system::error_code ec, std::size_t length) {
                if (!ec) {
                    do_server_work();
                }
            }
        );
    }
    void do_server_work() {
        string raw_request(data_);
        request_ = parse_request(raw_request);
        execute();
    }
    void execute() {
        signal(SIGCHLD, SIG_IGN);
        int pid;
        if ((pid = fork()) < 0) {
            cerr << "fork failed." << endl;
        }
        if (pid == 0) {
            set_env_variables();
            dup2(socket_.native_handle(), STDIN_FILENO);
            dup2(socket_.native_handle(), STDOUT_FILENO);
            char *argv[2];
            string uri = "." + request_.target;
            argv[0] = strdup(uri.c_str());
            argv[1] = NULL;
			socket_.close();
            cout << request_.protocol + " 200 OK\r\n" << flush;
            if (execvp(argv[0], argv) < 0) {
				exit(-1);
			}
        } else {
            socket_.close();
        }
    }
    void set_env_variables() {
        setenv("REQUEST_METHOD", request_.method.c_str(), 1);
        setenv("REQUEST_URI", request_.uri.c_str(), 1);
        setenv("QUERY_STRING", request_.query.c_str(), 1);
        setenv("SERVER_PROTOCOL", request_.protocol.c_str(), 1);
        setenv("HTTP_HOST", request_.host.c_str(), 1);
        setenv("SERVER_ADDR", socket_.local_endpoint().address().to_string().c_str(), 1);
        setenv("SERVER_PORT", to_string(socket_.local_endpoint().port()).c_str(), 1);
        setenv("REMOTE_ADDR", socket_.remote_endpoint().address().to_string().c_str(), 1);
        setenv("REMOTE_PORT", to_string(socket_.remote_endpoint().port()).c_str(), 1);
    }
    tcp::socket socket_;
    enum { max_length = 1024 };
    char data_[max_length];
    request request_;
};

class server {
public:
    server(boost::asio::io_context & io_context, short port): acceptor_(io_context, tcp::endpoint(tcp::v4(), port)) {
        do_accept();
    }
private:
    void do_accept() {
        acceptor_.async_accept (
            [this](boost::system::error_code ec, tcp::socket socket) { 
                if (!ec) {
                    std::make_shared<session>(std::move(socket))->start();
                }
            do_accept();
            }
        );
    }
    tcp::acceptor acceptor_;
};

request parse_request(string raw_request) {
    request request_;
    vector<string> lines;
    size_t start = 0;
    for (size_t i = 0; i < raw_request.size(); ++i) {
        if (raw_request[i] == '\r' && raw_request[i+1] == '\n' && start != i) {
            lines.push_back(raw_request.substr(start, i - start));
            i += 2;
            start = i;
        }
    }
    start = 0;
    int count = 0;
    string uri_query;
    for (size_t i = 0; i < lines[0].size(); ++i) {
        if (lines[0][i] == ' ') {
            if (count == 0) {
                request_.method = lines[0].substr(start, i - start);
                ++i;
                start = i;
                ++count;
            } else if (count == 1) {
                uri_query = lines[0].substr(start, i - start);
                request_.protocol = lines[0].substr(i+1);
                break;
            }
        }
    }
    request_.uri = uri_query;
    size_t idx = uri_query.find("?");
    if (idx == string::npos) {
        request_.target = request_.uri;
    } else {
        request_.target = uri_query.substr(0, idx);
        request_.query = uri_query.substr(idx+1);
    }
    for (size_t i = 1; i < lines.size(); ++i) {
        if (lines[i].substr(0, 4) == "Host") {
            request_.host = lines[i].substr(6);
        }
    }
    return request_;
}

int main(int argc, char* argv[])
{
    try {
        if (argc != 2) {
            std::cerr << "Usage: http_server <port>\n";
            return 1;
        }
        boost::asio::io_context io_context;
        server s(io_context, std::atoi(argv[1]));
        io_context.run();
    }
    catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
    }
    return 0;
}