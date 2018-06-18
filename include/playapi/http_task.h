#pragma once

#include "task.h"

namespace playapi {

class http_task : public task<http_response> {

private:
    http_request req;

public:
    http_task(http_request req) : req(std::move(req)) {
    }

    static task_ptr<http_response> make(http_request req) {
        return task_ptr<http_response>(new http_task(std::move(req)));
    }

    void call(std::function<void (http_response&&)> success, std::function<void (std::exception_ptr)> error) override {
        req.perform([success, error](http_response resp) {
            try {
                success(std::move(resp));
            } catch (std::exception& e) {
                error(std::current_exception());
            }
        }, error);
    }

    http_response call() override {
        return req.perform();
    }

};

}