#include <playapi/util/http_request_pool.h>
#include <functional>
#include <zconf.h>
#include <curl/multi.h>

using namespace playapi;

http_request_pool::http_request_pool() {
    curlm = curl_multi_init();

    loop = uv_loop_new();
    uv_timer_init(loop, &timeout_timer);
    timeout_timer.data = this;
    uv_async_init(loop, &notify_handle, [](uv_async_t* handle) {
        ((http_request_pool*) handle->data)->handle_interrupt();
    });
    notify_handle.data = this;
    thread = std::thread(std::bind(&http_request_pool::run, this));
}

http_request_pool::~http_request_pool() {
    mutex.lock();
    stopping = true;
    mutex.unlock();
    interrupt();
    thread.join();
    curl_multi_cleanup(curlm);
    uv_loop_delete(loop);
}

void http_request_pool::interrupt() {
    uv_async_send(&notify_handle);
}

void http_request_pool::handle_interrupt() {
    std::unique_lock<std::mutex> lock(mutex);
    if (stopping) {
        uv_stop(loop);
        return;
    }
    for (CURL* handle : add_queue)
        curl_multi_add_handle(curlm, handle);
    for (CURL* handle : remove_queue)
        curl_multi_remove_handle(curlm, handle);
}

http_request_pool::socket_data::socket_data(http_request_pool* pool, curl_socket_t s) : pool(pool), socket(s) {
    uv_poll_init_socket(pool->loop, &poll_handle, s);
    poll_handle.data = this;
}

void http_request_pool::socket_data::delete_later() {
    uv_close((uv_handle_t *) &poll_handle, [](uv_handle_t* handle) {
        delete (http_request_pool::socket_data*) handle->data;
    });
}

void http_request_pool::handle_socket_action(curl_socket_t socket, int flags) {
    int handles;
    curl_multi_socket_action(curlm, socket, flags, &handles);

    struct CURLMsg* m;
    while ((m = curl_multi_info_read(curlm, &handles))) {
        if (m->msg == CURLMSG_DONE) {
            base_entry* entry;
            curl_easy_getinfo(m->easy_handle, CURLINFO_PRIVATE, &entry);
            entry->done(m->easy_handle, m->data.result);
            delete entry;
            curl_easy_setopt(m->easy_handle, CURLOPT_PRIVATE, NULL);
        }
    }
}

int http_request_pool::curl_socket_func(CURL* curl, curl_socket_t s, int what, void* userp, void* socketp) {
    socket_data* socketd = (socket_data*) socketp;
    if (what == CURL_POLL_IN || what == CURL_POLL_OUT || what == CURL_POLL_INOUT) {
        if (socketd == nullptr) {
            socketd = new socket_data((http_request_pool*) userp, s);
            curl_multi_assign(((http_request_pool*) userp)->curlm, s, socketd);
        }
        int mask = 0;
        if (what == CURL_POLL_IN || what == CURL_POLL_INOUT)
            mask |= UV_READABLE;
        if (what == CURL_POLL_OUT || what == CURL_POLL_INOUT)
            mask |= UV_WRITABLE;
        uv_poll_start(&socketd->poll_handle, mask, [](uv_poll_t* handle, int status, int events) {
            socket_data* data = (socket_data*) handle->data;
            int flags = 0;
            if (events & UV_READABLE)
                flags |= CURL_CSELECT_IN;
            if (events & UV_WRITABLE)
                flags |= CURL_CSELECT_OUT;
            data->pool->handle_socket_action(data->socket, flags);
        });
    } else if (what == CURL_POLL_REMOVE && socketd) {
        uv_poll_stop(&socketd->poll_handle);
        socketd->delete_later();
        curl_multi_assign(((http_request_pool*) userp)->curlm, s, nullptr);
    }
    return 0;
}

int http_request_pool::curl_timer_func(CURLM* multi, long timeout_ms, void* userp) {
    if (timeout_ms <= 0) {
        uv_timer_stop(&((http_request_pool*) userp)->timeout_timer);
        if (timeout_ms == 0)
            ((http_request_pool*) userp)->handle_socket_action(CURL_SOCKET_TIMEOUT, 0);
    } else {
        uv_timer_start(&((http_request_pool*) userp)->timeout_timer, [](uv_timer_t* handle) {
            ((http_request_pool*) handle->data)->handle_socket_action(CURL_SOCKET_TIMEOUT, 0);
        }, timeout_ms, 0);
    }
}

void http_request_pool::run() {
    curl_multi_setopt(curlm, CURLMOPT_SOCKETFUNCTION, curl_socket_func);
    curl_multi_setopt(curlm, CURLMOPT_SOCKETDATA, this);
    curl_multi_setopt(curlm, CURLMOPT_TIMERFUNCTION, curl_timer_func);
    curl_multi_setopt(curlm, CURLMOPT_TIMERDATA, this);
    uv_run(loop, UV_RUN_DEFAULT);
}