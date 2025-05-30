#include "com_thread.h"

#include "socket.h"
#include <cassert>
#include <cstring>

static bool _running = true;
enum ComState { COM_IDLE, COM_WRITE, COM_READ, COM_QUERY };
static ComState _state = COM_IDLE;
static Socket *_socket = nullptr;
static size_t _messageLength = 0;
static int port = 0;
static const char *hostname = nullptr;

static void *_com_thread(void *args) {
    while (port == 0) {
        if (!_running) {
            return nullptr;
        }
        // std::this_thread::sleep_for(std::chrono::milliseconds{200});
    }
    Socket socket = hostname ? Socket::createClient(port, hostname) : Socket::createServer(port);
    _socket = &socket;
    // std::this_thread::sleep_for(std::chrono::milliseconds{10});
    while (_running) {
        switch (_state) {
        case COM_IDLE:
            break;
        case COM_QUERY:
        case COM_WRITE:
            socket.write(_messageLength);
            _messageLength = 0;
            _state = (_state == COM_QUERY) ? COM_READ : COM_IDLE;
            break;
        case COM_READ:
            socket.clear();
            _messageLength = socket.read();
            _state = COM_IDLE;
            break;
        }
        // std::this_thread::sleep_for(std::chrono::milliseconds{10});
    }
    _socket = nullptr;
    socket.close();
    return nullptr;
}

void bytesized_com::setPort(int port_) { port = port_; }
void bytesized_com::setPortAndHostname(int port_, const char *hostname_) {
    hostname = hostname_;
    port = port_;
}

bool bytesized_com::isRunning() { return _running; }

bool bytesized_com::isServer() { return _socket != nullptr && _socket->listen_fd != -1; }

bool bytesized_com::isConnected() { return _socket != nullptr && _socket->fd != -1; }

bool bytesized_com::isIdle() { return _state == COM_IDLE; }

bool bytesized_com::send(const char *data, size_t length) {
    if (!_running || _state != COM_IDLE) {
        return false;
    }
    assert(length < BYTESIZED_SOCKET_LENGTH);
    memcpy(_socket->buffer, data, length);
    _state = COM_WRITE;
    _messageLength = length;
    return true;
}

bool bytesized_com::query(const char *data, size_t length) {
    if (send(data, length)) {
        _state = COM_QUERY;
        return true;
    }
    return false;
}

bool bytesized_com::receive() {
    if (!_running || _state != COM_IDLE) {
        return false;
    }
    _state = COM_READ;
    return true;
}
bool bytesized_com::hasMessage() { return _state == COM_IDLE && _messageLength > 0; }
size_t bytesized_com::messageLength() { return _messageLength; }
std::string_view bytesized_com::getMessage() {
    if (_messageLength == 0 || _state != COM_IDLE) {
        return {};
    }
    size_t len = _messageLength;
    _messageLength = 0;
    return {_socket->buffer, len};
}

void bytesized_com::Thread::start() { pthread_create(&thread, NULL, _com_thread, NULL); }
void bytesized_com::Thread::stop() {
    _running = false;
    pthread_join(thread, NULL);
}