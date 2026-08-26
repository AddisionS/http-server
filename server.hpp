#pragma once

#include "http.hpp"
#include <string>
#include <map>

class Server {
public:
    Server(int port, Router& router);

    void run();

private:
    struct ClientState {
        std::string request_buffer;
        std::string response_buffer;
        bool close_after_write = false;
    };

    int port;
    Router& router;
    std::map<int, ClientState> clients;

    void closeClient(int fd, int epoll_fd);
};