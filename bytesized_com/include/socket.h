#pragma once

#include <cstddef>

#ifndef BYTESIZED_SOCKET_LENGTH
#define BYTESIZED_SOCKET_LENGTH 256
#endif

struct Socket {
    static Socket createServer(int port);
    static Socket createClient(int port, const char *hostname);

    int write(size_t length = BYTESIZED_SOCKET_LENGTH);
    int read();
    void close();
    void clear();

    int listen_fd = -1;
    int fd = -1;
    char buffer[BYTESIZED_SOCKET_LENGTH];
};