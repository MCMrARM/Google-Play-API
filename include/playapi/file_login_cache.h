#pragma once

#include "login_cache.h"
#include <unordered_map>
#include <mutex>

namespace playapi {

class file_login_cache : public login_cache {

private:
    std::mutex mutex;
    std::string path;
    std::map<std::pair<std::string, std::string>,
            std::pair<std::string, std::chrono::system_clock::time_point>> auth_cookies;

    void load();

    void save();

public:
    file_login_cache(std::string path) : path(std::move(path)) {
        load();
    }

    void clear() {
        {
            std::lock_guard<std::mutex> l (mutex);
            auth_cookies.clear();
        }
        save();
    }

    void cache(std::string const& service, std::string const& app, std::string const& token,
               std::chrono::system_clock::time_point expire) override {
        {
            std::lock_guard<std::mutex> l (mutex);
            auth_cookies[{service, app}] = {token, expire};
        }
        save();
    }

    std::string get_cached(std::string const& service, std::string const& app) override {
        std::lock_guard<std::mutex> l (mutex);
        auto ret = auth_cookies.find({service, app});
        if (ret != auth_cookies.end() && ret->second.second > std::chrono::system_clock::now())
            return ret->second.first;
        return std::string();
    }

};

}