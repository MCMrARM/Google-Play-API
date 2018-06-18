#pragma once

#include <string>
#include <vector>
#include <gsf.pb.h>
#include "task.h"

namespace playapi {

class device_info;
class login_api;

struct checkin_result {

    long long time = 0; // 0 if never
    unsigned long long android_id = 0;
    unsigned long long security_token = 0;
    std::string device_data_version_info;

    std::string get_string_android_id() const;

};

class checkin_api {

    device_info const& device;

    struct auth_user {
        std::string email, auth_cookie;
    };
    std::mutex auth_mutex;
    std::vector<auth_user> auth;

public:

    checkin_api(device_info const& device);

    task_ptr<void> add_auth(login_api& login);

    void clear_auth() {
        std::lock_guard<std::mutex> l (auth_mutex);
        auth.clear();
    }

    task_ptr<checkin_result> perform_checkin(const checkin_result& last_checkin = checkin_result());

};

}