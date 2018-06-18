#pragma once

#include <curl/curl.h>
#include <thread>
#include <mutex>
#include <vector>
#include <uv.h>

namespace playapi {

class http_request_pool {

private:
    struct socket_data {
        http_request_pool* pool;
        curl_socket_t socket;
        uv_poll_t poll_handle;

        socket_data(http_request_pool* pool, curl_socket_t s);

        void delete_later();
    };

    CURLM* curlm;
    std::thread thread;
    uv_loop_t loop;
    uv_timer_t timeout_timer;
    uv_async_t notify_handle;
    std::mutex mutex;
    bool stopping = false;
    std::vector<CURL*> add_queue, remove_queue;

    void run();

    void interrupt();

    void handle_interrupt();

    void handle_socket_action(curl_socket_t socket, int flags);

    static int curl_socket_func(CURL* curl, curl_socket_t s, int what, void* userp, void* socketp);

    static int curl_timer_func(CURLM* multi, long timeout_ms, void *userp);

public:
    static http_request_pool& default_instance() {
        static http_request_pool pool;
        return pool;
    }

    struct base_entry {
        virtual ~base_entry() = default;
        virtual void done(CURL* curl, CURLcode code) = 0;
    };

    http_request_pool();
    ~http_request_pool();

    CURLM* handle() { return curlm; }

    void add(CURL* curl) {
        mutex.lock();
        add_queue.push_back(curl);
        mutex.unlock();
        interrupt();
    }

    void remove(CURL* curl) {
        mutex.lock();
        remove_queue.push_back(curl);
        mutex.unlock();
        interrupt();
    }

};

}