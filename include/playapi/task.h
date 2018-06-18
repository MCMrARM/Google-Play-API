#pragma once

#include <string>
#include <map>
#include <chrono>
#include <functional>
#include <future>
#include "util/http.h"

namespace playapi {

template <typename T, typename ...Args>
class task;

template <typename T, typename ...Args>
using task_ptr = std::shared_ptr<task<T, Args...>>;

template <typename ...Args>
class task<void, Args...> : public std::enable_shared_from_this<task<void, Args...>> {

public:
    virtual ~task() {}
    virtual void call(Args&&... args, std::function<void ()> success,
                      std::function<void (std::exception_ptr)> error) = 0;
    virtual void call(Args&&... args) = 0;

    template <typename T2>
    task_ptr<T2, Args...> then(task_ptr<T2> task);

    template <typename T2>
    task_ptr<T2, Args...> then(std::function<T2 ()> task);

    template <typename T2>
    task_ptr<T2, Args...> then(std::function<task_ptr<T2> ()> task);

};

template <typename T, typename ...Args>
class task : public std::enable_shared_from_this<task<T, Args...>> {

public:
    virtual ~task() {}
    virtual void call(Args&&... args, std::function<void (T&&)> success,
                      std::function<void (std::exception_ptr)> error) = 0;
    virtual T call(Args&&... args) = 0;

    template <typename T2>
    task_ptr<T2, Args...> then(task_ptr<T2, T> task);

    template <typename T2>
    task_ptr<T2, Args...> then(std::function<T2 (T)> task);

    template <typename T2>
    task_ptr<T2, Args...> then(std::function<task_ptr<T2> (T)> task);

};

template <typename T>
class pre_set_task : public task<T> {

private:
    T value;

public:
    explicit pre_set_task(T value) : value(std::move(value)) {
    }

    static task_ptr<T> make(T value) {
        return task_ptr<T>(new pre_set_task(value));
    }

    void call(std::function<void (T&&)> success, std::function<void (std::exception_ptr)> error) {
        auto valuet = value;
        success(std::move(valuet));
    }

    T call() {
        return value;
    }

};

template <typename T, typename ...Args>
class function_task;

template <typename ...Args>
class function_task<void, Args...> : public task<void, Args...> {

private:
    std::function<void (Args&&...)> func;

public:
    function_task(std::function<void (Args&&...)> func) : func(std::move(func)) {
    }

    static task_ptr<void, Args...> make(std::function<void (Args&&...)> func) {
        return task_ptr<void, Args...>(new function_task(func));
    }

    void call(Args&&... args, std::function<void ()> success, std::function<void (std::exception_ptr)> error) override {
        try {
            func(std::forward<Args...>(args)...);
            success();
        } catch (std::exception& e) {
            error(std::current_exception());
        }
    }

    void call(Args&&... args) override {
        return func(std::forward<Args...>(args)...);
    }

};

template <typename T, typename ...Args>
class function_task : public task<T, Args...> {

private:
    std::function<T (Args&&...)> func;

public:
    function_task(std::function<T (Args&&...)> func) : func(std::move(func)) {
    }

    static task_ptr<T, Args...> make(std::function<T (Args&&...)> func) {
        return task_ptr<T, Args...>(new function_task(func));
    }

    void call(Args&&... args, std::function<void (T&&)> success, std::function<void (std::exception_ptr)> error) override {
        try {
            success(func(std::forward<Args...>(args)...));
        } catch (std::exception& e) {
            error(std::current_exception());
        }
    }

    T call(Args&&... args) override {
        return func(std::forward<Args...>(args)...);
    }

};

template <typename T, typename ...Args>
class flat_function_task : public task<T, Args...> {

private:
    std::function<task_ptr<T> (Args&&...)> func;

public:
    flat_function_task(std::function<task_ptr<T> (Args&&...)> func) : func(std::move(func)) {
    }

    static task_ptr<T, Args...> make(std::function<task_ptr<T> (Args&&...)> func) {
        return task_ptr<T, Args...>(new flat_function_task(func));
    }

    void call(Args&&... args, std::function<void (T&&)> success, std::function<void (std::exception_ptr)> error) override {
        task_ptr<T> ret;
        try {
            ret = func(std::forward<Args...>(args)...);
            if (!ret)
                throw std::runtime_error("flat_function_task's function must return a valid pointer");
        } catch (std::exception& e) {
            error(std::current_exception());
            return;
        }
        ret->call(success, error);
    }

    T call(Args&&... args) override {
        return func(std::forward<Args...>(args)...)->call();
    }

};

template <typename T, typename T2, typename ...Args>
class merged_task;

template <typename T, typename ...Args>
class merged_task<T, void, Args...> : public task<void, Args...> {

private:
    task_ptr<T, Args...> t1;
    task_ptr<void, T> t2;

public:
    merged_task(task_ptr<T, Args...> t1, task_ptr<void, T> t2) : t1(std::move(t1)), t2(std::move(t2)) {
    }

    void call(Args&&... args, std::function<void ()> success,
              std::function<void (std::exception_ptr)> error) override {
        auto t2 = this->t2;
        t1->call(args..., [t2, success, error](T&& t) {
            t2->call(std::move(t), success, error);
        }, [error](std::exception_ptr e) {
            error(e);
        });
    }

    void call(Args&&... args) override {
        return t2->call(t1->call(args...));
    }

};

template <typename T2, typename ...Args>
class merged_task<void, T2, Args...> : public task<T2, Args...> {

private:
    task_ptr<void, Args...> t1;
    task_ptr<T2> t2;

public:
    merged_task(task_ptr<void, Args...> t1, task_ptr<T2> t2) : t1(std::move(t1)), t2(std::move(t2)) {
    }

    void call(Args&&... args, std::function<void (T2&&)> success,
              std::function<void (std::exception_ptr)> error) override {
        auto t2 = this->t2;
        t1->call(args..., [t2, success, error]() {
            t2->call(success, error);
        }, [error](std::exception_ptr e) {
            error(e);
        });
    }

    T2 call(Args&&... args) override {
        t1->call(args...);
        return t2->call();
    }

};

template <typename T, typename T2, typename ...Args>
class merged_task : public task<T2, Args...> {

private:
    task_ptr<T, Args...> t1;
    task_ptr<T2, T> t2;

public:
    merged_task(task_ptr<T, Args...> t1, task_ptr<T2, T> t2) : t1(std::move(t1)), t2(std::move(t2)) {
    }

    void call(Args&&... args, std::function<void (T2&&)> success,
              std::function<void (std::exception_ptr)> error) override {
        auto t2 = this->t2;
        t1->call(args..., [t2, success, error](T&& t) {
            t2->call(std::move(t), success, error);
        }, [error](std::exception_ptr e) {
            error(e);
        });
    }

    T2 call(Args&&... args) override {
        return t2->call(t1->call(args...));
    }

};

template <typename ...Args>
template <typename T2>
inline task_ptr<T2, Args...> task<void, Args...>::then(task_ptr<T2> task) {
    return task_ptr<T2, Args...>(new merged_task<void, T2, Args...>(this->shared_from_this(), std::move(task)));
}

template <typename ...Args>
template <typename T2>
inline task_ptr<T2, Args...> task<void, Args...>::then(std::function<T2 ()> task) {
    return this->then(function_task<T2>::make(std::move(task)));
}

template <typename ...Args>
template <typename T2>
inline task_ptr<T2, Args...> task<void, Args...>::then(std::function<task_ptr<T2> ()> task) {
    return this->then(flat_function_task<T2>::make(std::move(task)));
}

template <typename T, typename ...Args>
template <typename T2>
inline task_ptr<T2, Args...> task<T, Args...>::then(task_ptr<T2, T> task) {
    return task_ptr<T2, Args...>(new merged_task<T, T2, Args...>(this->shared_from_this(), std::move(task)));
}

template <typename T, typename ...Args>
template <typename T2>
inline task_ptr<T2, Args...> task<T, Args...>::then(std::function<T2 (T)> task) {
    return this->then(function_task<T2, T>::make(std::move(task)));
}

template <typename T, typename ...Args>
template <typename T2>
inline task_ptr<T2, Args...> task<T, Args...>::then(std::function<task_ptr<T2> (T)> task) {
    return this->then(flat_function_task<T2, T>::make(std::move(task)));
}

}