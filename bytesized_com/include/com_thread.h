#pragma once

#include <pthread.h>
#include <string_view>

namespace bytesized_com {

void setPort(int port);
void setPortAndHostname(int port, const char *hostname);
bool isRunning();
bool isServer();
bool isConnected();
bool isIdle();
bool send(const char *data, size_t length);
bool query(const char *data, size_t length);
bool receive();
bool hasMessage();
size_t messageLength();
std::string_view getMessage();

struct Thread {
    void start();
    void stop();
    pthread_t thread;
};

} // namespace bytesized_com
