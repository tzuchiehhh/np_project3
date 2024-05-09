#include <cstdlib>
#include <iostream>
#include <memory>
#include <utility>
#include <boost/asio.hpp>
#include <boost/filesystem.hpp>
#include <string>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sstream>
#include <regex>

using boost::asio::ip::tcp;
using namespace std;

class session
    : public std::enable_shared_from_this<session> {
public:
    session(tcp::socket socket)
        : socket_(std::move(socket)) {
    }

    void start() {
        do_read();
    }

private:
    tcp::socket socket_;
    enum { max_length = 1024 };
    char data_[max_length];

    string request_method;
    string request_uri;
    string query_string;
    string server_protocol;
    string http_host;
    string server_addr;
    string server_port;
    string remote_addr;
    string remote_port;

    void do_read() {
        auto self(shared_from_this());
        socket_.async_read_some(boost::asio::buffer(data_, max_length),
                                [this, self](boost::system::error_code ec, std::size_t length) {
                                    if (!ec) {
                                        parse_request();
                                        do_write(length);
                                    }
                                });
    }

    void do_write(std::size_t length) {
        auto self(shared_from_this());
        boost::asio::async_write(socket_, boost::asio::buffer(data_, length),
                                 [this, self](boost::system::error_code ec, std::size_t /*length*/) {
                                     if (!ec) {
                                         do_read();
                                     }
                                 });
    }

    void parse_request() {
        cout << data_ << endl;
        string request = string(data_);
        stringstream request_ss(request);
        string header;
        getline(request_ss, header);
        stringstream header_ss(header);
        string request_uri_line;
        header_ss >> request_method >> request_uri_line >> server_protocol;
        stringstream request_uri_ss(request_uri_line);
        getline(request_uri_ss, request_uri);
        getline(request_uri_ss, query_string);
        
        string header_host;
        getline(request_ss, header_host);
        stringstream host_ss(header);
        string tmp;
        host_ss >> tmp >> http_host;

        server_addr = socket_.local_endpoint().address().to_string();
        server_port = to_string(socket_.local_endpoint().port());
        remote_addr = socket_.remote_endpoint().address().to_string();
        remote_port = to_string(socket_.remote_endpoint().port());
        
        cout << "request_method: " << request_method << endl;
        cout << "request_uri: " << request_uri << endl;
        cout << "query_string: " << query_string << endl;
        cout << "server_protocol: " << server_protocol << endl;
        cout << "http_host: " << http_host << endl;
        cout << "server_addr: " << server_addr << endl;
        cout << "server_port: " << server_port << endl;
        cout << "remote_addr: " << remote_addr << endl;
        cout << "remote_port: " << remote_port << endl;
    }
};

class server {
public:
    server(boost::asio::io_context &io_context, short port)
        : acceptor_(io_context, tcp::endpoint(tcp::v4(), port)) {
        do_accept();
    }

private:
    void do_accept() {
        acceptor_.async_accept(
            [this](boost::system::error_code ec, tcp::socket socket) {
                if (!ec) {
                    std::make_shared<session>(std::move(socket))->start();
                }

                do_accept();
            });
    }

    tcp::acceptor acceptor_;
};

int main(int argc, char *argv[]) {
    try {
        if (argc != 2) {
            std::cerr << "Usage: async_tcp_echo_server <port>\n";
            return 1;
        }

        boost::asio::io_context io_context;

        server s(io_context, std::atoi(argv[1]));

        io_context.run();
    } catch (std::exception &e) {
        std::cerr << "Exception: " << e.what() << "\n";
    }

    return 0;
}