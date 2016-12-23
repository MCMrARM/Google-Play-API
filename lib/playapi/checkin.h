#pragma once

#include <string>
#include <vector>

namespace playapi {

class device_info;
class login_api;

class checkin_api {

    device_info& device;

    struct auth_user {
        std::string email, auth_cookie;
    };
    std::vector<auth_user> auth;

public:

    checkin_api(device_info& device);

    void add_auth(login_api& login);

    void clear_auth() { auth.clear(); }

    void perform();

};

}