#include "socks_server.h"

boost::asio::io_context io_context;

class session: public std::enable_shared_from_this<session> {
public:
	session(tcp::socket socket): socket_(std::move(socket)), server_socket_(io_context), acceptor_(io_context, tcp::endpoint(tcp::v4(), 0)){

	}
	void start() {
		read_request();
	}
private:
	void read_request() {
		auto self(shared_from_this());
		socket_.async_read_some(boost::asio::buffer(data_, MAX_LENGTH), 
            [this, self](boost::system::error_code ec, std::size_t length) {
                if (!ec) {
                    do_parse(length);
					if (request_.VN == 4) {
						permit = check_pemission(request_);
						if (permit) {
							if (request_.CD == 1) {
								connect();
							} else if (request_.CD == 2) {
								bind();
							}
						}
					} else {
						permit = false;
					}
					print_message();
					send_reply();
                }
            }
        );
	}
	void do_parse(size_t length) {
		request_ = parse_request(data_);
		if (!strncmp(request_.ip.c_str(), "0.0.0.", 6)) { // Socks4A
			string new_host;
			size_t i;
			for (i = 8; i < length; i++) {
				if (data_[i] == 0) {
					break;
				}
			}
			for (++i; i < length; i++) {
				if (data_[i] == 0) {
					break;
				}
				new_host += data_[i];
			}
			tcp::resolver resolver_(io_context);
			boost::system::error_code ec;
			tcp::resolver::results_type result = resolver_.resolve(new_host, to_string(request_.port), ec);
			
			for (auto it = result.begin(); it != result.end(); it++) {
				request_.ip = it->endpoint().address().to_string();
			}
		}
	}
	void connect() {
		auto self(shared_from_this());
		tcp::endpoint endpoint_(boost::asio::ip::address::from_string(request_.ip), request_.port);
		server_socket_.open(tcp::v4());
		server_socket_.async_connect(endpoint_, 
			[this, self](const boost::system::error_code &ec) {
				if (!ec) {
					read_server();
					read_client();
				}
			}
		);
	}
	void bind() {
		auto self(shared_from_this());
		acceptor_.async_accept(
			[this, self](const boost::system::error_code &ec, tcp::socket sock) {
				if (!ec) {
					uint8_t reply[8];
					memset(reply, 0, sizeof(reply));
					if (socket_.remote_endpoint().address().to_string() == request_.ip) {
						server_socket_ = move(sock);
						reply[1] = 90;
						socket_.write_some(boost::asio::buffer(reply, 8)); 
					} else {
						reply[1] = 91;
						socket_.write_some(boost::asio::buffer(reply, 8)); 
					}
					read_server();
					read_client();
				} else {

				}
			}
		);
	}
	void read_server() {
		auto self(shared_from_this());
		server_socket_.async_read_some(boost::asio::buffer(server_buffer_, MAX_LENGTH), 
			[this, self](const boost::system::error_code &ec, size_t length) {
				if (!ec) {
					write_client(length);
				} else {
					server_socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, error);
					socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, error);
				}
			}
		);
	}
	void write_client(size_t length) {
		auto self(shared_from_this());
		async_write(socket_, boost::asio::buffer(server_buffer_, length),
			[this, self] (const boost::system::error_code &ec, size_t length) {
				if (!ec) {
					read_server();
				} else {
					server_socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, error);
					socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, error);
				}
			}
		);
	}
	void read_client() {
		auto self(shared_from_this());
		socket_.async_read_some(boost::asio::buffer(client_buffer_, MAX_LENGTH),
			[this, self] (const boost::system::error_code &ec, size_t length) {
				if (!ec) {
					write_server(length);
				} else {
					server_socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, error);
					socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, error);
				}
			}
		);
	}

	void write_server(size_t length) {
		auto self(shared_from_this());
		async_write(server_socket_, boost::asio::buffer(client_buffer_, length),
			[this, self] (const boost::system::error_code &ec, size_t length) {
				if (!ec) {
					read_client();
				} else {
					server_socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, error);
					socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, error);
				}
			}
		);
	}

	void print_message() {
		cout << "<S_IP>: " << socket_.remote_endpoint().address().to_string() << endl;
		cout << "<S_PORT>: " << socket_.remote_endpoint().port() << endl;
		cout << "<D_IP>: " << request_.ip << endl;
		cout << "<D_PORT>: "  << request_.port << endl;
		if (request_.CD == 1) {
			cout << "<Command>: CONNECT" << endl;
		} else if (request_.CD == 2) {
			cout << "<Command>: BIND" << endl;
		}
		if (permit) {
			cout << "<Reply>: Accept" << endl;
		} else {
			cout << "<Reply>: Reject" << endl;
		}
		cout << endl;
	}
	void send_reply() {
		uint8_t reply[8];
		memset(reply, 0, sizeof(reply));
		if (permit) {
			reply[1] = 90;
		} else {
			reply[1] = 91;
		}
		if (request_.CD == 2) {
			int port = acceptor_.local_endpoint().port();
			reply[2] = port / 256;
			reply[3] = port % 256;
			array<unsigned char, 4> ip = socket_.local_endpoint().address().to_v4().to_bytes();
			for (int i = 0; i < 4; ++i) {
				reply[4+i] = int(ip[i]);
			}
		}     
		socket_.write_some(boost::asio::buffer(reply, 8)); 
	}
	tcp::socket socket_;
	tcp::socket server_socket_;
	tcp::acceptor acceptor_;
	uint8_t data_[MAX_LENGTH];
	char server_buffer_[MAX_LENGTH];
	char client_buffer_[MAX_LENGTH];
	boost::system::error_code error;
	struct request request_;
	bool permit;
};

class server {
public:
	server(short port): acceptor_(io_context, tcp::endpoint(tcp::v4(), port)) {
		signal(SIGCHLD, SIG_IGN);
		do_accept(); 
	}
private:
	void do_accept() {
		acceptor_.async_accept(
            [this](boost::system::error_code ec, tcp::socket socket) { 
                if (!ec) {
					io_context.notify_fork(boost::asio::io_context::fork_prepare);
					while ((pid_ = fork()) == -1);
					if (pid_ == 0) {
						io_context.notify_fork(boost::asio::io_context::fork_child);
						std::make_shared<session>(std::move(socket))->start();
					} else {
						io_context.notify_fork(boost::asio::io_context::fork_parent);
						do_accept();
                	}
            	} else {
					do_accept();
				}
			}
        );
	}
	tcp::acceptor acceptor_;
	int pid_;
};

struct request parse_request(uint8_t *data_) {
    int VN = int(data_[0]);
    int CD = int(data_[1]);
    int port = int(data_[2])*256 + int(data_[3]);
    string ip;
    for(int i = 4; i < 8; ++i) {
        ip += (to_string(int(data_[i])) + '.');
    }
    ip = ip.erase(ip.length()-1);
    struct request request_(VN, CD, port, ip);
    return request_;
}

bool check_pemission(struct request request_) {
	ifstream f("socks.conf");
	if (!f) {
		cerr << "Open failed." << endl;
	}
	string line;
	bool permit = false;
	if (request_.CD == 1) {
		while(getline(f, line)) {
			if (line[7] == 'c') { // connect
				size_t idx = line.find('*');
				size_t length = idx - 9;
				if (strncmp(request_.ip.c_str(), line.substr(9, length).c_str(), length) == 0) {
					permit = true;
				}
				break;
			}
		}
	} else if (request_.CD == 2) {
		while(getline(f, line)) {
			if (line[7] == 'b') { // bind
				size_t idx = line.find('*');
				size_t length = idx - 9;
				if (strncmp(request_.ip.c_str(), line.substr(9, length).c_str(), length) == 0) {
					permit = true;
				}
				break;
			}
		}
	}
	return permit;
}

int main(int argc, char* argv[]) {
    try {
		if (argc != 2) {
			std::cerr << "Usage: socks_server <port>\n";
			return -1;
		}
		server s(std::stoi(argv[1]));
		io_context.run();
	}
	catch (std::exception &e) {
		std::cerr << "exception " << e.what() << '\n';
	}
	return 0;
}