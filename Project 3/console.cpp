#include "console.h"

class client: public std::enable_shared_from_this<client> {
public:
    client(const shared_ptr<tcp::socket> socket, const struct client_info cli_, int id): socket_(std::move(socket)), client_info_(cli_), id_(id) {
        
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
        string output = "<script>document.getElementById(\"s" + to_string(id_) + "\").innerHTML += \"" + escape(message) + "\" ;</script>";
        cout << output << flush;
    }
    void output_command(string command) {
        string output = "<script>document.getElementById(\"s" + to_string(id_) + "\").innerHTML += \"<b>" + escape(command) + "</b>\" ;</script>";
        cout << output << flush;
    }
    shared_ptr<tcp::socket> socket_;
    client_info client_info_;
    int id_;
    ifstream file;
    char read_buffer[BUFFER_MAX_LEN];
};

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



void print_body(vector<client_info> client_infos) {
    cout << 
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
            cout << "<th scope=\"col\">" << client_infos[i].host_name << ":" << client_infos[i].port <<  "</th>";
        } else {
            break;
        }
    }
    cout << 
    "</thead> \
      <tbody> \
        <tr>";
    for (size_t i = 0; i < client_infos.size(); ++i) {
        if (client_infos[i].host_name.empty()) {
            break;
        }
        cout << "<td><pre id=\"s" << i << "\" class=\"mb-0\"></pre></td>";
    }
    cout << 
    "</tr> \
      </tbody> \
    </table> \
  </body> \
</html>" << flush;
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
        string query = getenv("QUERY_STRING");
        vector<client_info> client_infos = parse_query(query);
        boost::asio::io_context io_context;
        tcp::resolver resolver_(io_context);
        cout << "Content-Type: text/html\r\n\r\n";
        print_body(client_infos);
        for (size_t i = 0; i < 5; i++) {
            if (client_infos[i].host_name.empty()) {
                break;
            }
            shared_ptr<tcp::socket> sock = make_shared<tcp::socket>(io_context);
            resolver_.async_resolve(client_infos[i].host_name, to_string(client_infos[i].port),
				[sock, client_infos, i](const boost::system::error_code& ec1, tcp::resolver::results_type results) {
					if (!ec1) {
						for (auto it = results.begin(); it != results.end(); it++) {
							sock->async_connect(it->endpoint(), 
                                [sock, client_infos, i](const boost::system::error_code &ec2) {
                                    if (!ec2) {
                                        make_shared<client>(std::move(sock), client_infos[i], i)->start();
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
    catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";;
    }
    return 0;
}