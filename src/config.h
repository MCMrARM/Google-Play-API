#pragma once

#include <string>
#include <map>

namespace playdl { struct device_info; }

struct app_config {

    std::string user_email, user_token;

    void load();

    void save();

    void load_device(playdl::device_info& device);

    void save_device(playdl::device_info& device);

};