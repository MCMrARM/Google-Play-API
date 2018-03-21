#pragma once

#include <memory>
#include <string>
#include <vector>
#include "checkin.h"

namespace playapi {

class device_info;

class mcs_registration_api {

private:
    device_info& device;
    checkin_result checkin_data;
    int next_legacy_register_id = 0;

    static const char* const REGISTER_URL;

public:

    struct registration_request {
        std::string app_id;
        std::string app_cert;
        std::string app_ver;
        std::vector<std::string> senders;
        std::map<std::string, std::string> extras;
    };
    struct legacy_registration_request {
        std::string app_id;
        std::string app_ver;
        std::string app_ver_name;
        std::string sender;
    };

    mcs_registration_api(device_info& device) : device(device) {}

    void set_checkin_data(const checkin_result& result);

    std::string perform_register(registration_request const& request);

    std::string perform_legacy_register(legacy_registration_request const& request);

};

}