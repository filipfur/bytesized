#include "socket.h"

#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

Socket Socket::createClient(int port, const char *hostname) {
    struct sockaddr_in serv_addr;
    struct hostent *server;
    Socket socket;
    socket.fd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (socket.fd < 0) {
        perror("ERROR opening socket");
        exit(1);
    }
    server = gethostbyname(hostname);
    if (server == NULL) {
        fprintf(stderr, "ERROR, no such host\n");
        exit(1);
    }
    bzero((char *)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
    serv_addr.sin_port = htons(port);
    if (connect(socket.fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("ERROR connecting");
        exit(1);
    }
    return socket;
}

Socket Socket::createServer(int port) {
    Socket socket;
    struct sockaddr_in serv_addr, cli_addr;
    socklen_t clilen;
    socket.listen_fd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (socket.listen_fd < 0) {
        perror("ERROR opening socket");
        exit(1);
    }
    bzero((char *)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(port);
    if (bind(socket.listen_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("ERROR on binding");
        exit(1);
    }
    listen(socket.listen_fd, 5);
    clilen = sizeof(cli_addr);
    socket.fd = accept(socket.listen_fd, (struct sockaddr *)&cli_addr, &clilen);
    if (socket.fd < 0) {
        perror("ERROR on accept");
        exit(1);
    }
    return socket;
}

int Socket::write(size_t length) {
    int n = ::write(fd, buffer, length);
    if (n < 0) {
        perror("ERROR writing to socket");
        exit(1);
    }
    return n;
}

int Socket::read() {
    int n = ::read(fd, buffer, sizeof(buffer) - 1);
    if (n < 0) {
        perror("ERROR reading from socket");
        exit(1);
    }
    return n;
}

void Socket::close() {
    if (listen_fd != -1) {
        ::close(listen_fd);
    }
    ::close(fd);
}

void Socket::clear() { bzero(buffer, sizeof(buffer)); }