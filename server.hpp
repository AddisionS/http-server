#pragma once

#include "http.hpp"
#include <string>
#include <map>

class Server {
public:
    Server(int port);

    void run();

    void get(const std::string& path, Handler handler);
    void post(const std::string& path, Handler handler);
    void put(const std::string& path, Handler handler);
    void del(const std::string& path, Handler handler);

private:
    struct ClientState {
        std::string request_buffer;
        std::string response_buffer;
        bool close_after_write = false;
    };

    int port;
    Router router;
    std::map<int, ClientState> clients;

    void closeClient(int fd, int epoll_fd);
};