#include "socket.h"
#include <netdb.h>
#include <unistd.h>
#include <stdexcept>

using namespace playapi;

socket::~socket() {
    if (fd != -1)
        ::close(fd);
}

void socket::connect(std::string const& hostname, unsigned short port) {
    struct addrinfo* res;
    int error = getaddrinfo(hostname.c_str(), nullptr, nullptr, &res);
    if (error != 0)
        throw std::runtime_error("getaddrinfo() error: " + std::to_string(error));
    for (auto it = res; it != nullptr; it = it->ai_next) {
        if (it->ai_family != AF_INET && it->ai_family != AF_INET6)
            continue;
        if (it->ai_family == AF_INET)
            ((sockaddr_in*) it->ai_addr)->sin_port = ntohs(port);
        else if (it->ai_family == AF_INET6)
            ((sockaddr_in6*) it->ai_addr)->sin6_port = ntohs(port);
        connect(it->ai_addr, it->ai_addrlen);
        return;
    }
    free(res);
}

void socket::connect(struct sockaddr* addr, socklen_t addrlen) {
    if (fd != -1)
        throw std::runtime_error("The socket already has an associated fd");
    fd = ::socket(addr->sa_family, SOCK_STREAM, 0);
    if (fd < 0)
        throw std::runtime_error("socket() call failed " + std::to_string(fd));
    int ret = ::connect(fd, addr, addrlen);
    if (ret < 0)
        throw std::runtime_error("connect() call failed " + std::to_string(ret));
}

ssize_t playapi::socket::read(void* data, size_t datalen) {
    return ::read(fd, data, datalen);
}

ssize_t playapi::socket::write(const void* data, size_t datalen) {
    return ::write(fd, data, datalen);
}