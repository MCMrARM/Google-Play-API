#pragma once

#include <string>
#include <sys/socket.h>

namespace playapi {

class stream {

public:
    virtual ssize_t read(void* data, size_t datalen) = 0;
    virtual ssize_t write(const void* data, size_t datalen) = 0;

};

}