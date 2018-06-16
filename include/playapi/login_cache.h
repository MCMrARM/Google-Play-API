#pragma once

#include <string>
#include <map>
#include <chrono>

namespace playapi {

class login_cache {

public:
    virtual void cache(std::string const& service, std::string const& app, std::string const& token,
                       std::chrono::system_clock::time_point expire) = 0;

    virtual std::string get_cached(std::string const& service, std::string const& app) = 0;

};

}