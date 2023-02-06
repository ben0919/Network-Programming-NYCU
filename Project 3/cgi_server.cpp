#include "cgi_server.h"

boost::asio::io_context io_context;
class client: public enable_shared_from_this<client> {
public:
    client(const shared_ptr<tcp::socket> socket, const struct client_info cli_, int id, shared_ptr<tcp::socket> write_sock): socket_(move(socket)), client_info_(cli_), id_(id), write_sock_(write_sock) {
        
    }
    void start() {
        open_file();
        do_read();
    }
private:
    void open_file() {
        string file_name = "test_case/" + client_info_.file_name;
        file.open(file_name);
    }

    void do_read() {
        auto self(shared_from_this());
        memset(read_buffer, 0, sizeof(read_buffer));
        memset(write_buffer, 0, sizeof(write_buffer));
        socket_->async_read_some(boost::asio::buffer(read_buffer, BUFFER_MAX_LEN),
            [this, self] (const boost::system::error_code &ec, size_t length) {
                string message = string(read_buffer);
                output_shell(message);
                if (message.find("%") == string::npos) {
                    do_read();
                } else {
                    do_write();
                }
            }
        );
    }

    void do_write() {
        auto self(shared_from_this());
		string command = read_command();
		socket_->async_write_some(boost::asio::buffer(command.c_str(), command.size()),
            [this, self, command](const boost::system::error_code &ec, size_t length) {
                output_command(command);
                if (strncmp(command.c_str(), "exit", 4) != 0) {
                    do_read();
                } else {
                    do_exit();
                }
			}
        );
    }

    void do_exit() {
        socket_->close();
        if (write_sock_.use_count() == 2) {
            write_sock_->close();
        }
    }

    string read_command() {
        string command;
		getline(file, command);
		size_t idx;
	    if ((idx = command.find("\r")) != string::npos) {
		    command.erase(idx);
		}
		command += "\n";
		return command;
    }

    void output_shell(string message) {
        auto self(shared_from_this());
        string output = "<script>document.getElementById(\"s" + to_string(id_) + "\").innerHTML += \"" + escape(message) + "\" ;</script>";
        strcpy(write_buffer, output.c_str());
        boost::asio::async_write(
            *write_sock_, boost::asio::buffer(write_buffer, strlen(write_buffer)),
            [this, self](boost::system::error_code ec, size_t length) {
                
            }
        );
    }
    void output_command(string command) {
        auto self(shared_from_this());
        string output = "<script>document.getElementById(\"s" + to_string(id_) + "\").innerHTML += \"<b>" + escape(command) + "</b>\" ;</script>";
        strcpy(write_buffer, output.c_str());
        boost::asio::async_write(
            *write_sock_, boost::asio::buffer(write_buffer, strlen(write_buffer)),
            [this, self](boost::system::error_code ec, size_t length) {
                
            }
        );
    }
    shared_ptr<tcp::socket> socket_;
    client_info client_info_;
    int id_;
    shared_ptr<tcp::socket> write_sock_;
    ifstream file;
    char read_buffer[BUFFER_MAX_LEN];
    char write_buffer[BUFFER_MAX_LEN];
};

class session: public enable_shared_from_this<session> {
public:
    session(tcp::socket socket): socket_(move(socket)) {

    }
    void start() {
        do_read();
    }
private:
    void do_read() {
        auto self(shared_from_this());
        socket_.async_read_some(boost::asio::buffer(input, BUFFER_MAX_LEN), 
            [this, self](boost::system::error_code ec, size_t length) {
                if (!ec) {
                    do_server_work();
                }
            }
        );
    }
    void do_server_work() {
        string raw_request(input);
        request_ = parse_request(raw_request);
        save_env_variables();
        
        execute();
    }
    void execute() {
        auto self(shared_from_this());
        if (env_variables["REQUEST_URI"].find("panel.cgi") != string::npos) {
            string message = env_variables["SERVER_PROTOCOL"] + " 200 OK\r\n\r\n";
            strcpy(output, message.c_str());
            boost::asio::async_write(
                socket_, boost::asio::buffer(output, strlen(output)),
                [this, self](boost::system::error_code ec, size_t /*length*/) {
                    if (!ec) {
                        write_panel_html();
                    }
                }
            );
        }
        if (env_variables["REQUEST_URI"].find("console.cgi") != string::npos) {
            run_console();
        }
    }
    void save_env_variables() {
        env_variables["REQUEST_METHOD"] = request_.method;
        env_variables["REQUEST_URI"] = request_.uri;
        env_variables["QUERY_STRING"] = request_.query;
        env_variables["SERVER_PROTOCOL"] = request_.protocol;
        env_variables["HTTP_HOST"] = request_.host;
        env_variables["SERVER_ADDR"] = socket_.local_endpoint().address().to_string();
        env_variables["SERVER_PORT"] = to_string(socket_.local_endpoint().port());
        env_variables["REMOTE_ADDR"] = socket_.remote_endpoint().address().to_string();
        env_variables["REMOTE_PORT"] = to_string(socket_.remote_endpoint().port());
    }
    void write_panel_html() {
        auto self(shared_from_this());
        string message, line;
        ifstream panel_html("panel.html");
        while (getline(panel_html, line)) {
            message += (line + "\n");
        }
        strcpy(output, message.c_str());
        boost::asio::async_write(
            socket_, boost::asio::buffer(output, strlen(output)),
            [this, self](boost::system::error_code ec, size_t /*length*/) {
                if (!ec) {

                }
            }
        );
    }
    void run_console() {
        shared_ptr<tcp::socket> write_sock(&socket_);
        string query = env_variables["QUERY_STRING"];
        vector<client_info> client_infos = parse_query(query);
        write_console_html(client_infos);
        tcp::resolver resolver_(io_context);
        for (size_t i = 0; i < 5; i++) {
            if (client_infos[i].host_name.empty()) {
                break;
            }
            shared_ptr<tcp::socket> sock = make_shared<tcp::socket>(io_context);
            resolver_.async_resolve(client_infos[i].host_name, to_string(client_infos[i].port),
				[sock, client_infos, i, write_sock](const boost::system::error_code& ec1, tcp::resolver::results_type results) {
					if (!ec1) {
						for (auto it = results.begin(); it != results.end(); it++) {
							sock->async_connect(it->endpoint(), 
                                [sock, client_infos, i, write_sock](const boost::system::error_code &ec2) {
                                    if (!ec2) {
                                        make_shared<client>(move(sock), client_infos[i], i, write_sock)->start();
                                    }
                                }
                            );	
						}
					} else {
						cerr << "resolve failed" << endl;
					}
				}
            );
	    }	
        io_context.run();
    }
    void write_console_html(vector<client_info> client_infos) {
        auto self(shared_from_this());
        string message = request_.protocol + " 200 OK\r\n";
        message += "Content-Type: text/html\r\n\r\n";
        message += 
        "<!DOCTYPE html> \
            <html lang=\"en\"> \
            <head> \
                <meta charset=\"UTF-8\" /> \
                <title>NP Project 3 Sample Console</title> \
                <link \
                rel=\"stylesheet\" \
                href=\"https://cdn.jsdelivr.net/npm/bootstrap@4.5.3/dist/css/bootstrap.min.css\"  \
                integrity=\"sha384-TX8t27EcRE3e/ihU7zmQxVncDAy5uIKz4rEkgIXeMed4M0jlfIDPvg6uqKI2xXr2\" \
                crossorigin=\"anonymous\"  \
                /> \
                <link \
                href=\"https://fonts.googleapis.com/css?family=Source+Code+Pro\" \
                rel=\"stylesheet\" \
                /> \
                <link \
                rel=\"icon\" \
                type=\"image/png\" \
                href=\"https://cdn0.iconfinder.com/data/icons/small-n-flat/24/678068-terminal-512.png\" \
                /> \
                <style> \
                * { \
                    font-family: 'Source Code Pro', monospace; \
                    font-size: 1rem !important; \
                } \
                body { \
                    background-color: #212529; \
                } \
                pre { \
                    color: #cccccc; \
                } \
                b { \
                    color: #01b468; \
                } \
                </style> \
            </head> \
            <body> \
                <table class=\"table table-dark table-bordered\"> \
                <thead> \
                    <tr>";
        for (size_t i = 0; i < client_infos.size(); ++i) {
            if (!client_infos[i].host_name.empty()) {
                message += ("<th scope=\"col\">" + client_infos[i].host_name + ":" + to_string(client_infos[i].port) + "</th>");
            } else {
                break;
            }
        }
        message += "</thead> \
        <tbody> \
            <tr>";
        for (size_t i = 0; i < client_infos.size(); ++i) {
            if (client_infos[i].host_name.empty()) {
                break;
            }
            message += ("<td><pre id=\"s" + to_string(i) + "\" class=\"mb-0\"></pre></td>");
        }
        message += "</tr> \
            </tbody> \
        </table> \
        </body> \
        </html>";
        strcpy(output, message.c_str());
        boost::asio::async_write(
            socket_, boost::asio::buffer(output, strlen(output)),
            [this, self](boost::system::error_code ec, size_t /*length*/) {
                if (!ec) {

                }
            }
        );
    }
    tcp::socket socket_;
    char input[BUFFER_MAX_LEN];
    char output[BUFFER_MAX_LEN];
    request request_;
    map<string, string> env_variables;
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
                    make_shared<session>(move(socket))->start();
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

vector<client_info> parse_query(string query) {
    query += "&"; 
    vector<client_info> client_infos;
    size_t start = 0;
    client_info client_info_;
    for (size_t i = 0; i < query.size(); ++i) {
        if (query[i] == '&' || i == query.size() - 1) {
            if (query[start] == 'h' && i - start > 3) {
                client_info_.host_name = query.substr(start + 3, i - start - 3);
            }
            if (query[start] == 'p' && i - start > 3) {
                client_info_.port = stoi(query.substr(start + 3, i - start - 3));
            }
            if (query[start] == 'f' && i - start > 3) {
                client_info_.file_name = query.substr(start + 3, i - start - 3);
                client_infos.push_back(client_info_);
            }
            ++i;
            start = i;
        }
    }
    for (size_t i = client_infos.size(); i < 5; ++i) {
        client_info client_info_;
        client_infos.push_back(client_info_);
    }
    return client_infos;
}

string escape(string message) {
    string result;
	for (size_t i = 0; i < message.size(); ++i) {
		switch (message[i]) {
			case '\n':
				result += "&NewLine;";
				break;
			case '\r':
				result += "";
				break;
			case '&':
				result += "&amp;";
				break;
			case '>':
				result += "&gt;";
				break;
			case '<':
				result += "&lt;";
				break;
			default:
				result += message[i]; 
		}
	}
	return result;
}

int main(int argc, char* argv[])
{
    try {
        if (argc != 2) {
            cerr << "Usage: cgi_server <port>\n";
            return 1;
        }
        server s(io_context, atoi(argv[1]));
        io_context.run();
    }
    catch (exception& e) {
        cerr << "Exception: " << e.what() << "\n";
    }
    return 0;
}