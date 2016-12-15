#pragma once

#include "login.h"

namespace playdl {

class checkin_handler {

    device_info& device;
    login_manager login;

public:

    checkin_handler(device_info& device);

    login_manager& get_login() { return login; }

    void do_checkin();

};

}