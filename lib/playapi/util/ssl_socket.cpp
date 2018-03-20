#include <stdexcept>
#include "ssl_socket.h"

using namespace playapi;

ssl_socket::ssl_socket(playapi::socket& socket) : socket(socket) {
    const SSL_METHOD* method = SSLv23_method();
    if (!method)
        throw std::runtime_error("SSL Error: Method is null");
    ssl_ctx = SSL_CTX_new(method);
    if (!ssl_ctx)
        throw std::runtime_error("SSL Error: Context is null");
    ssl = SSL_new(ssl_ctx);
    if (!ssl)
        throw std::runtime_error("SSL Error: SSL is null");
    SSL_set_fd(ssl, socket.get_fd());
    int ret = SSL_connect(ssl);
    if (ret <= 0)
        throw std::runtime_error("SSL Error: SSL_connect returned an error");
}

ssl_socket::~ssl_socket() {
    SSL_free(ssl);
    SSL_CTX_free(ssl_ctx);
}

ssize_t ssl_socket::read(void* data, size_t datalen) {
    return SSL_read(ssl, data, (int) datalen);
}

ssize_t ssl_socket::write(const void* data, size_t datalen) {
    return SSL_write(ssl, data, (int) datalen);
}