#pragma once

#include <string>
#include <sys/socket.h>
#include "stream.h"

namespace playapi {

class socket : public stream {

private:

    int fd = -1;

public:
    socket() {}

    socket(struct sockaddr* addr, socklen_t addrlen) { connect(addr, addrlen); }

    socket(std::string const& hostname, unsigned short port) { connect(hostname, port); }

    ~socket();

    void connect(struct sockaddr* addr, socklen_t addrlen);

    void connect(std::string const& hostname, unsigned short port);


    ssize_t read(void* data, size_t datalen) override;

    ssize_t write(const void* data, size_t datalen) override;

};

}