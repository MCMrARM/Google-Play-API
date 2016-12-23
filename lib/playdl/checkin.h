#pragma once

#include <vector>
#include "login.h"

namespace playdl {

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