#pragma once

#include <string>
#include <openssl/ssl.h>
#include "socket.h"

namespace playapi {

class ssl_socket : public stream {

private:
    playapi::socket& socket;
    SSL_CTX* ssl_ctx;
    SSL* ssl;

public:
    ssl_socket(playapi::socket& socket);

    ~ssl_socket();


    ssize_t read(void* data, size_t datalen) override;

    ssize_t write(const void* data, size_t datalen) override;

};

}