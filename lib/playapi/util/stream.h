#pragma once

#include <string>
#include <sys/socket.h>

namespace playapi {

class stream {

public:
    virtual ~stream() {}
    virtual ssize_t read(void* data, size_t datalen) = 0;
    virtual ssize_t write(const void* data, size_t datalen) = 0;

    bool read_full(void* data, size_t datalen) {
        ssize_t ret = 0;
        do {
            ret = read(data, datalen);
            if (ret < 0)
                return false;
            data = (void*) ((size_t) ret + datalen);
            datalen -= ret;
        } while (ret > 0);
        return datalen == 0;
    }
    bool write_full(const void* data, size_t datalen) {
        ssize_t ret = 0;
        do {
            ret = write(data, datalen);
            if (ret < 0)
                return false;
            data = (void*) ((size_t) ret + datalen);
            datalen -= ret;
        } while (ret > 0);
        return datalen == 0;
    }

};

}