#include <iostream>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "http.hpp"
#include <stdexcept>
#include <algorithm>

using namespace std;

int main() {

    Router router;

    router.addRoute(
        "GET",
        "/hello",
        [](const Request&, Response& res) {
            res.status_code = StatusCode::OK;
            res.body = "Hello from router!";
            res.headers["Content-Type"] = "text/plain";
        }
    );

    // creating file descriptor for socket connection
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (server_fd < 0) {
        perror("socket failed");
        return 1;
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
    address.sin_port = htons(8080);

    // bind port and ip
    if (bind(
        server_fd,
        (sockaddr*)&address,
        sizeof(address)
    ) < 0) {
        perror("bind failed");
        return 1;
    }

    // listen for incoming TCP requests
    if (listen(server_fd, 10) < 0) {
        perror("listen failed");
        return 1;
    }

    cout << "Listening on port 8080...\n";

    // accepting connections
    while (true) {

        sockaddr_in client_addr{};
        socklen_t client_len = sizeof(client_addr);

        int client_fd = accept(
            server_fd,
            (sockaddr*)&client_addr,
            &client_len
        );

        if (client_fd < 0) {
            perror("accept failed");
            continue;
        }

        char ip_str[INET_ADDRSTRLEN];

        inet_ntop(
            AF_INET,
            &client_addr.sin_addr,
            ip_str,
            sizeof(ip_str)
        );

        cout << "New connection from "
             << ip_str
             << "\n";

        bool keep_alive = true;

        // Persistent buffer for this TCP connection
        string raw_request;

        // handle multiple requests on the same connection
        while (keep_alive) {

            // Read until we have complete headers
            while (raw_request.find("\r\n\r\n") == string::npos) {

                char buffer[4096];

                ssize_t bytes_read = recv(
                    client_fd,
                    buffer,
                    sizeof(buffer),
                    0
                );

                if (bytes_read > 0) {

                    raw_request.append(
                        buffer,
                        bytes_read
                    );

                } else if (bytes_read == 0) {

                    // client closed connection
                    keep_alive = false;
                    break;

                } else {

                    perror("recv failed");
                    keep_alive = false;
                    break;
                }
            }

            if (!keep_alive) {
                break;
            }

            try {

                // parse HTTP request
                Request request =
                    parseRequest(raw_request);

                auto connection =
                    request.headers.find("Connection");

                if (
                    connection != request.headers.end() &&
                    connection->second == "close"
                ) {
                    keep_alive = false;
                }

                size_t headers_end =
                    raw_request.find("\r\n\r\n");

                size_t body_start =
                    headers_end + 4;

                size_t content_length = 0;

                auto it =
                    request.headers.find("Content-Length");

                if (it != request.headers.end()) {

                    content_length =
                        std::stoul(it->second);

                    
                    size_t body_bytes_received =
                        raw_request.size() - body_start;

    
                    size_t body_bytes_needed =
                        content_length;

                    if (body_bytes_received < body_bytes_needed) {

                        size_t remaining =
                            body_bytes_needed -
                            body_bytes_received;

                        while (remaining > 0) {

                            char buffer[4096];

                            ssize_t bytes_read = recv(
                                client_fd,
                                buffer,
                                std::min(
                                    remaining,
                                    sizeof(buffer)
                                ),
                                0
                            );

                            if (bytes_read <= 0) {

                                throw runtime_error(
                                    "Incomplete request body"
                                );
                            }

                            raw_request.append(
                                buffer,
                                bytes_read
                            );

                            remaining -= bytes_read;
                        }
                    }

                    request.body =
                        raw_request.substr(
                            body_start,
                            content_length
                        );
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
                    router.pathExists(request.path)
                ) {

                    response.status_code =
                        StatusCode::MethodNotAllowed;

                    response.body =
                        "Method Not Allowed";

                    response.headers["Content-Type"] =
                        "text/plain";

                } else {

                    response.status_code =
                        StatusCode::NotFound;

                    response.body =
                        "Not Found";

                    response.headers["Content-Type"] =
                        "text/plain";
                }

                response.headers["Content-Length"] =
                    std::to_string(
                        response.body.size()
                    );

                if (!keep_alive) {
                    response.headers["Connection"] = "close";
                }

                // serialize response
                string raw_response =
                    serializeResponse(response);

                // send response
                send(
                    client_fd,
                    raw_response.c_str(),
                    raw_response.size(),
                    0
                );

                
                size_t request_size =
                    body_start + content_length;

                raw_request.erase(
                    0,
                    request_size
                );

            }
            catch (const std::exception& e) {

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

                response.headers["Content-Type"] =
                    "text/plain";

                response.headers["Content-Length"] =
                    std::to_string(
                        response.body.size()
                    );

                string raw_response =
                    serializeResponse(response);

                send(
                    client_fd,
                    raw_response.c_str(),
                    raw_response.size(),
                    0
                );

                
                keep_alive = false;
            }
        }

        // connection is finally closed
        close(client_fd);
    }

    close(server_fd);

    return 0;
}