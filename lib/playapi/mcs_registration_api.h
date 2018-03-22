#pragma once

#include <memory>
#include <string>
#include <vector>
#include "checkin.h"

namespace playapi {

class device_info;

class mcs_registration_api {

private:
    friend class mcs_connection;
    device_info& device;
    checkin_result checkin_data;
    int next_token_request_id = 0;

    static const char* const REGISTER_URL;

public:

    struct registration_request {
        std::string app_pkg_name;
        std::string app_cert;
        std::string app_ver;
        std::vector<std::string> senders;
        std::map<std::string, std::string> extras;
    };
    struct instance_id_request {
        std::string app_pkg_name;
        std::string app_cert;
        std::string app_id;
        std::string gmp_app_id;
        std::string app_ver;
        std::string app_ver_name;
        std::string sender;
        std::string scope = "*";
        std::string pubkey;
        std::string sig;
        std::string client_id = "iid-12221000";
    };

    mcs_registration_api(device_info& device) : device(device) {}

    void set_checkin_data(const checkin_result& result);

    std::string perform_register(registration_request const& request);

    std::string obtain_instance_id_token(instance_id_request const& request);

    /*
     * Legacy registration:
     * (internally used fields: sender, X-google.message_id, app, device, cert, app_ver, gcm_ver)
     * - single sender, has app id, app cert and app ver set
     * - extras:
     *   `google.message_id=google.rpc` + an autoincrementing integer, starting from 1 (e.g. `google.message_id=google.rpc1`)
     *   no other extras
     */

    /*
     * InstanceID token acquisition
     * (internally used fields: X-subtype, X-X-subscription, sender, X-X-subtype, X-app_ver, X-kid, X-osv, X-sig, X-cliv, X-gmsv, X-pub2, X-X-kid, X-appid, X-scope, X-subscription, X-gmp_app_id, X-app_ver_name, app, device, cert, app_ver, gcm_ver)
     */

};

}