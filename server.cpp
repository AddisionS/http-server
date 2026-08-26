#include "server.hpp"

#include <iostream>
#include <sys/socket.h>
#include <unistd.h>
#include <stdexcept>
#include <algorithm>
#include <fcntl.h>
#include <sys/epoll.h>
#include <cerrno>
#include <arpa/inet.h>

using namespace std;

Server::Server(int port, Router& router)
    : port(port), router(router) {}

void Server::closeClient(int fd, int epoll_fd) {

    epoll_ctl(
        epoll_fd,
        EPOLL_CTL_DEL,
        fd,
        nullptr
    );

    clients.erase(fd);

    close(fd);
}

void Server::run() {

    // creating file descriptor for socket connection

    int server_fd = socket(
        AF_INET,
        SOCK_STREAM,
        0
    );

    if (server_fd < 0) {
        perror("socket failed");
        return;
    }

    // making server socket non-blocking

    int flags = fcntl(
        server_fd,
        F_GETFL,
        0
    );

    if (flags < 0) {
        perror("fcntl F_GETFL failed");
        close(server_fd);
        return;
    }

    if (fcntl(
        server_fd,
        F_SETFL,
        flags | O_NONBLOCK
    ) < 0) {
        perror("fcntl F_SETFL failed");
        close(server_fd);
        return;
    }

    // set socket option for instant reusability after restart

    int opt = 1;

    setsockopt(
        server_fd,
        SOL_SOCKET,
        SO_REUSEADDR,
        &opt,
        sizeof(opt)
    );

    // socket address struct declaration

    sockaddr_in address{};

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    // bind port and ip

    if (bind(
        server_fd,
        (sockaddr*)&address,
        sizeof(address)
    ) < 0) {
        perror("bind failed");
        close(server_fd);
        return;
    }

    // listen for incoming TCP requests

    if (listen(server_fd, 10) < 0) {
        perror("listen failed");
        close(server_fd);
        return;
    }

    cout << "Listening on port "
         << port
         << "...\n";

    // create epoll instance

    int epoll_fd = epoll_create1(0);

    if (epoll_fd < 0) {
        perror("epoll_create1 failed");
        close(server_fd);
        return;
    }

    // register server socket with epoll

    epoll_event event{};

    event.events = EPOLLIN;
    event.data.fd = server_fd;

    if (epoll_ctl(
        epoll_fd,
        EPOLL_CTL_ADD,
        server_fd,
        &event
    ) < 0) {
        perror("epoll_ctl failed");
        close(epoll_fd);
        close(server_fd);
        return;
    }

    // epoll event loop

    constexpr int MAX_EVENTS = 64;
    epoll_event events[MAX_EVENTS];

    while (true) {

        int event_count = epoll_wait(
            epoll_fd,
            events,
            MAX_EVENTS,
            -1
        );

        if (event_count < 0) {

            if (errno == EINTR) {
                continue;
            }

            perror("epoll_wait failed");
            break;
        }

        for (int i = 0; i < event_count; i++) {

            int fd = events[i].data.fd;

            // accepting new connections

            if (fd == server_fd) {

                int client_fd = accept(
                    server_fd,
                    nullptr,
                    nullptr
                );

                if (client_fd < 0) {

                    if (errno != EAGAIN &&
                        errno != EWOULDBLOCK) {
                        perror("accept failed");
                    }

                    continue;
                }

                // making client socket non-blocking

                int client_flags = fcntl(
                    client_fd,
                    F_GETFL,
                    0
                );

                if (client_flags < 0) {
                    perror("fcntl F_GETFL failed");
                    close(client_fd);
                    continue;
                }

                if (fcntl(
                    client_fd,
                    F_SETFL,
                    client_flags | O_NONBLOCK
                ) < 0) {
                    perror("fcntl F_SETFL failed");
                    close(client_fd);
                    continue;
                }

                // register client with epoll

                epoll_event client_event{};

                client_event.events = EPOLLIN;
                client_event.data.fd = client_fd;

                if (epoll_ctl(
                    epoll_fd,
                    EPOLL_CTL_ADD,
                    client_fd,
                    &client_event
                ) < 0) {
                    perror("epoll_ctl client failed");
                    close(client_fd);
                    continue;
                }

                // create state for client

                clients[client_fd] = ClientState{};

                cout << "New client: "
                     << client_fd
                     << '\n';

                continue;
            }

            // existing client has data ready

            if (events[i].events & EPOLLIN) {

                char buffer[4096];

                ssize_t bytes_read = recv(
                    fd,
                    buffer,
                    sizeof(buffer),
                    0
                );

                if (bytes_read > 0) {

                    clients[fd].request_buffer.append(
                        buffer,
                        bytes_read
                    );

                    string& raw_request =
                        clients[fd].request_buffer;

                    // process complete requests

                    while (true) {

                        size_t headers_end =
                            raw_request.find(
                                "\r\n\r\n"
                            );

                        if (headers_end ==
                            string::npos) {
                            break;
                        }

                        try {

                            // parse HTTP request

                            Request request =
                                parseRequest(
                                    raw_request
                                );

                            size_t body_start =
                                headers_end + 4;

                            size_t request_length =
                                body_start;

                            // read request body

                            auto it =
                                request.headers.find(
                                    "Content-Length"
                                );

                            if (it !=
                                request.headers.end()) {

                                size_t content_length =
                                    std::stoul(
                                        it->second
                                    );

                                size_t body_bytes_received =
                                    raw_request.size()
                                    - body_start;

                                if (body_bytes_received <
                                    content_length) {
                                    break;
                                }

                                request.body =
                                    raw_request.substr(
                                        body_start,
                                        content_length
                                    );

                                request_length =
                                    body_start
                                    + content_length;
                            }

                            // keep alive

                            bool keep_alive = true;

                            auto connection =
                                request.headers.find(
                                    "Connection"
                                );

                            if (connection !=
                                request.headers.end() &&
                                connection->second ==
                                "close") {

                                keep_alive = false;
                            }

                            cout << "Method: "
                                 << request.method
                                 << '\n';

                            cout << "Path: "
                                 << request.path
                                 << '\n';

                            cout << "Version: "
                                 << request.version
                                 << '\n';

                            // response generation

                            Response response;

                            response.version =
                                "HTTP/1.1";

                            Handler* handler =
                                router.findRoute(
                                    request.method,
                                    request.path
                                );

                            if (handler) {

                                (*handler)(
                                    request,
                                    response
                                );

                            } else if (
                                router.pathExists(
                                    request.path
                                )
                            ) {

                                response.status_code =
                                    StatusCode::MethodNotAllowed;

                                response.body =
                                    "Method Not Allowed";

                                response.headers[
                                    "Content-Type"
                                ] = "text/plain";

                            } else {

                                response.status_code =
                                    StatusCode::NotFound;

                                response.body =
                                    "Not Found";

                                response.headers[
                                    "Content-Type"
                                ] = "text/plain";
                            }

                            // response headers

                            response.headers[
                                "Content-Length"
                            ] = std::to_string(
                                response.body.size()
                            );

                            if (!keep_alive) {

                                response.headers[
                                    "Connection"
                                ] = "close";

                                clients[fd]
                                    .close_after_write = true;
                            }

                            // serialize response

                            string raw_response =
                                serializeResponse(
                                    response
                                );

                            clients[fd]
                                .response_buffer +=
                                raw_response;

                            // remove processed request

                            raw_request.erase(
                                0,
                                request_length
                            );

                            // try sending response

                            string& response_buffer =
                                clients[fd]
                                    .response_buffer;

                            ssize_t bytes_sent = send(
                                fd,
                                response_buffer.data(),
                                response_buffer.size(),
                                0
                            );

                            if (bytes_sent > 0) {

                                response_buffer.erase(
                                    0,
                                    bytes_sent
                                );

                            } else if (
                                bytes_sent < 0 &&
                                errno != EAGAIN &&
                                errno != EWOULDBLOCK
                            ) {

                                perror("send failed");

                                closeClient(
                                    fd,
                                    epoll_fd
                                );

                                break;
                            }

                            // enable EPOLLOUT if data remains

                            if (!response_buffer.empty()) {

                                epoll_event write_event{};

                                write_event.events =
                                    EPOLLIN | EPOLLOUT;

                                write_event.data.fd = fd;

                                epoll_ctl(
                                    epoll_fd,
                                    EPOLL_CTL_MOD,
                                    fd,
                                    &write_event
                                );

                            } else if (
                                clients[fd]
                                    .close_after_write
                            ) {

                                closeClient(
                                    fd,
                                    epoll_fd
                                );

                                break;

                            } else {

                                epoll_event read_event{};

                                read_event.events =
                                    EPOLLIN;

                                read_event.data.fd = fd;

                                epoll_ctl(
                                    epoll_fd,
                                    EPOLL_CTL_MOD,
                                    fd,
                                    &read_event
                                );
                            }

                            if (!keep_alive) {
                                break;
                            }

                        }
                        catch (
                            const std::exception& e
                        ) {

                            cerr << "Bad request: "
                                 << e.what()
                                 << '\n';

                            Response response;

                            response.version =
                                "HTTP/1.1";

                            response.status_code =
                                StatusCode::BadRequest;

                            response.body =
                                "Bad Request";

                            response.headers[
                                "Content-Type"
                            ] = "text/plain";

                            response.headers[
                                "Content-Length"
                            ] = std::to_string(
                                response.body.size()
                            );

                            string raw_response =
                                serializeResponse(
                                    response
                                );

                            clients[fd]
                                .response_buffer +=
                                raw_response;

                            epoll_event write_event{};

                            write_event.events =
                                EPOLLIN | EPOLLOUT;

                            write_event.data.fd = fd;

                            epoll_ctl(
                                epoll_fd,
                                EPOLL_CTL_MOD,
                                fd,
                                &write_event
                            );

                            raw_request.clear();

                            break;
                        }
                    }

                } else if (bytes_read == 0) {

                    // client closed connection

                    cout << "Client "
                         << fd
                         << " disconnected\n";

                    closeClient(
                        fd,
                        epoll_fd
                    );

                    continue;

                } else {

                    if (errno != EAGAIN &&
                        errno != EWOULDBLOCK) {

                        perror("recv failed");

                        closeClient(
                            fd,
                            epoll_fd
                        );
                    }
                }
            }

            // existing client can accept outgoing data

            if (events[i].events & EPOLLOUT) {

                string& response_buffer =
                    clients[fd].response_buffer;

                ssize_t bytes_sent = send(
                    fd,
                    response_buffer.data(),
                    response_buffer.size(),
                    0
                );

                if (bytes_sent > 0) {

                    response_buffer.erase(
                        0,
                        bytes_sent
                    );

                } else if (
                    bytes_sent < 0 &&
                    errno != EAGAIN &&
                    errno != EWOULDBLOCK
                ) {

                    perror("send failed");

                    closeClient(
                        fd,
                        epoll_fd
                    );

                    continue;
                }

                if (response_buffer.empty()) {

                    if (clients[fd]
                        .close_after_write) {

                        closeClient(
                            fd,
                            epoll_fd
                        );

                        continue;
                    }

                    epoll_event read_event{};

                    read_event.events =
                        EPOLLIN;

                    read_event.data.fd = fd;

                    epoll_ctl(
                        epoll_fd,
                        EPOLL_CTL_MOD,
                        fd,
                        &read_event
                    );
                }
            }
        }
    }

    close(epoll_fd);
    close(server_fd);
}