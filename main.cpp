#include <iostream>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>

int main() {
    //creating file descriptor for socket connection
    int server_fd = socket(AF_INET, SOCK_STREAM ,0);
    if (server_fd < 0) {
        perror("socket failed");
        return 1;
    }

    //set socket option for instant reusability after restart
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    //socket address struct declaration
    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;  // listen on all interfaces
    address.sin_port = htons(8080);        // host-to-network byte order

    //bind port and ip 
    if (bind(server_fd, (sockaddr*)&address, sizeof(address)) < 0) {
        perror("bind failed");
        return 1;
    }

    //listening to incoming tcp request with a backlog queue of 10 
    if (listen(server_fd, 10) < 0) {
        perror("listen failed");
        return 1;
    }
    std::cout << "Listening on port 8080...\n";

    //accepting connections 
    while(true) {
        sockaddr_in client_addr{};
        socklen_t client_len = sizeof(client_addr);
        int client_fd = accept(server_fd, (sockaddr*)&client_addr, &client_len);
        if (client_fd < 0) {
            perror("accept failed");
            continue;
        }

        char ip_str[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, ip_str, sizeof(ip_str));
        std::cout << "New connection from " << ip_str << "\n";

        //read what the client sends
        char buffer[4096] = {0};
        ssize_t bytes_read = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
        if (bytes_read > 0) {
            std::cout << "--- Received " << bytes_read << " bytes ---\n";
            std::cout << buffer << "\n";
            std::cout << "-------------------------------\n";
        }

        //echo everthing back 
        send(client_fd, buffer, bytes_read, 0);

        //close client connection 
        close(client_fd);
    }
    close(server_fd);
    return 0;
}