#pragma once

#include <string>
#include <map>
#include <chrono>
#include <functional>
#include <future>
#include "util/http.h"

namespace playapi {

template <typename T>
class request {

private:
    http_request req;
    std::function<T (http_response)> cf;

public:
    using handle = void*;

    request(http_request req, std::function<T (http_response)> cf) : req(std::move(req)), cf(std::move(cf)) {
    }

    handle call(std::function<void (T)> success, std::function<void (std::exception_ptr)> error) {
        auto cf = this->cf;
        return req.perform([success, error, cf](http_response resp) {
            try {
                success(cf(std::move(resp)));
            } catch (std::exception& e) {
                error(std::current_exception());
            }
        }, error);
    }

    T call() {
        return cf(req.perform());
    }


};

}