#include <array>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/write.hpp>
#include <fstream>
#include <iostream>
#include <queue>
#include <string>
#include <tuple>
#include <vector>

using boost::asio::ip::tcp;
using namespace std;

boost::asio::io_context io_context;

class session
    : public std::enable_shared_from_this<session> {
public:
    enum { max_length = 10240 };
    // char input_data[max_length];
    session(tcp::socket socket, tcp::resolver::query query, string index, string filename)
        : socket_(std::move(socket)), resolver_(io_context), query_(move(query)) {
        index_ = index;
        commands = read_file_commands(filename);
    }

    void start() {
        auto self(shared_from_this());
        resolver_.async_resolve(query_, [this, self](boost::system::error_code ec, tcp::resolver::iterator iter) {
            if (!ec) {
                do_connect(iter);
            }
        });
    }

    void do_connect(tcp::resolver::iterator iter) {
        auto self(shared_from_this());
        socket_.async_connect(*iter, [this, self](boost::system::error_code ec) {
            if (!ec) {
                do_read();
            }
        });
    }

private:
    tcp::socket socket_;
    tcp::resolver resolver_;
    tcp::resolver::query query_;
    // enum { max_length = 10240 };
    char data_[max_length];
    string index_;
    queue<string> commands;

    void do_read() {
        auto self(shared_from_this());
        socket_.async_read_some(boost::asio::buffer(data_, max_length),
                                [this, self](boost::system::error_code ec, std::size_t length) {
                                    if (!ec) {
                                      cout<<"do_read!!!!!!!!!!!!!!!!!!!"<<endl;
                                        string res = string(data_);
                                        cout << res << endl;
                                        memset(data_, '\0', max_length);
                                        output_shell(res);
                                        if (res.find("%") != std::string::npos) {
                                            cout << "find %!!!!!!!!!!!!!!!!!!!!!!" << endl;
                                        } else {
                                            do_read();
                                        }
                                    }
                                });
    }

    void do_write(std::size_t length) {
        auto self(shared_from_this());
        boost::asio::async_write(socket_, boost::asio::buffer(data_, length),
                                 [this, self](boost::system::error_code ec, std::size_t /*length*/) {
                                     if (!ec) {
                                         io_context.notify_fork(boost::asio::io_context::fork_prepare);
                                     }
                                 });
    }

    queue<string> read_file_commands(string filename) {
        queue<string> commands;
        string line;
        ifstream file;
        file.open(filename);
        while (getline(file, line)) {
            commands.push(line + "\n");
        }
        file.close();
        return commands;
    }

    void output_shell(string str) {
        escape(str);
        // cout << "<script>document.getElementById(\'s" + index_ + "\').innerHTML += \'" + str + "\';</script>&NewLine;";
        cout<<str<<endl;
    }

    void escape(string &str) {
        boost::replace_all(str, "&", "&amp;");
        boost::replace_all(str, "\"", "&quot;");
        boost::replace_all(str, "\'", "&apos;");
        boost::replace_all(str, "<", "&lt;");
        boost::replace_all(str, ">", "&gt;");
        boost::replace_all(str, "\n", "&NewLine;");
        boost::replace_all(str, "\r", "");
    }
};

vector<tuple<string, string, string>> parse_query_string() {
    string query_string = string(getenv("QUERY_STRING"));
    cout << query_string << endl
         << endl;
    stringstream ss(query_string);
    string token;
    // 0: hostname; 1: port; 2:file
    vector<tuple<string, string, string>> clients;
    for (int i = 0; i < 5; i++) {
        string hostname;
        string port;
        string filename;
        getline(ss, hostname, '&');
        getline(ss, port, '&');
        getline(ss, filename, '&');
        hostname = hostname.substr(3);
        port = port.substr(3);
        filename = filename.substr(3);

        if (hostname.length() != 0 && port.length() != 0 && filename.length() != 0) {
            clients.push_back({hostname, port, "test_case/" + filename});
        }
    }

    // for(int i=0;i<clients.size();i++){
    //     cout<<"hostname: "<<get<0>(clients[i])<<" port: "<<get<1>(clients[i])<<" file: "<<get<2>(clients[i])<<endl;

    // }

    return clients;
}

void print_html(vector<tuple<string, string, string>> clients) {
    cout << "Content-type: text/html\r\n\r\n";
    cout << "<!DOCTYPE html>\
<html lang=\"en\">\
  <head>\
    <meta charset=\"UTF-8\" />\
    <title>NP Project 3 Sample Console</title>\
    <link\
      rel=\"stylesheet\"\
      href=\"https://cdn.jsdelivr.net/npm/bootstrap@4.5.3/dist/css/bootstrap.min.css\"\
      integrity=\"sha384-TX8t27EcRE3e/ihU7zmQxVncDAy5uIKz4rEkgIXeMed4M0jlfIDPvg6uqKI2xXr2\"\
      crossorigin=\"anonymous\"\
    />\
    <link\
      href=\"https://fonts.googleapis.com/css?family=Source+Code+Pro\"\
      rel=\"stylesheet\"\
    />\
    <link\
      rel=\"icon\"\
      type=\"image/png\"\
      href=\"https://cdn0.iconfinder.com/data/icons/small-n-flat/24/678068-terminal-512.png\"\
    />\
    <style>\
      * {\
        font-family: 'Source Code Pro', monospace;\
        font-size: 1rem !important;\
      }\
      body {\
        background-color: #212529;\
      }\
      pre {\
        color: #cccccc;\
      }\
      b {\
        color: #01b468;\
      }\
    </style>\
  </head>\
  <body>\
    <table class=\"table table-dark table-bordered\">\
      <thead>\
        <tr>";
    for (int i = 0; i < clients.size(); i++) {
        cout << "           <th scope=\"col\">" << get<0>(clients[i]) << ":" << get<1>(clients[i]) << "</th>";
    }
    cout << "         </tr>\
                </thead>\
                <tbody>\
                    <tr>";
    for (int i = 0; i < clients.size(); i++) {
        cout << "           <td><pre id=\"s" + to_string(i) + "\" class=\"mb-0\"></pre></td>";
    }
    cout << "         </tr>\
                </tbody>\
            </table>\
        </body>\
    </html>";
}

void connect_to_server(vector<tuple<string, string, string>> clients) {
    for (int i = 0; i < clients.size(); ++i) {
        tcp::resolver::query query(get<0>(clients[i]), get<1>(clients[i]));
        tcp::socket socket(io_context);
        make_shared<session>(move(socket), move(query), get<2>(clients[i]), to_string(i))->start();
    }
}

int main() {
    try {
        vector<tuple<string, string, string>> clients = parse_query_string();
        // print_html(clients);
        connect_to_server(clients);
        io_context.run();
    } catch (exception &e) {
        cerr << e.what() << endl;
    }
}
