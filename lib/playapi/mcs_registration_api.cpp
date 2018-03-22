#include "mcs_registration_api.h"

#include <sstream>
#include "util/http.h"
#include "device_info.h"

using namespace playapi;
const char* const mcs_registration_api::REGISTER_URL = "https://android.clients.google.com/c2dm/register3";

void mcs_registration_api::set_checkin_data(const checkin_result& result) {
    this->checkin_data = result;
}

std::string mcs_registration_api::perform_register(registration_request const& request) {
    //AidLogin
    http_request req (REGISTER_URL);
    req.set_user_agent("Android-GCM/1.5 (" + device.build_product + " " + device.build_id + ")");
    req.add_header("Authorization", "AidLogin " + std::to_string(checkin_data.android_id) + ":" +
            std::to_string(checkin_data.security_token));
    req.add_header("app", request.app_pkg_name);
    req.add_header("gcm_ver", std::to_string(device.build_google_services));

    url_encoded_entity body;

    {
        std::stringstream senders;
        bool sender_first = true;
        for (auto const& p : request.senders) {
            if (!sender_first)
                senders << ",";
            senders << p;
            sender_first = false;
        }
        body.add_pair("sender", senders.str());
    }

    body.add_pair("app", request.app_pkg_name);
    body.add_pair("device", std::to_string(checkin_data.android_id));
    if (!request.app_cert.empty())
        body.add_pair("cert", request.app_cert);
    if (!request.app_ver.empty())
        body.add_pair("app_ver", request.app_ver);
    body.add_pair("info", checkin_data.version_info);
    body.add_pair("gcm_ver", std::to_string(device.build_google_services));
    for (auto const& p : request.extras)
        body.add_pair("X-" + p.first, p.second);
    req.set_body(body);
    auto resp = req.perform();
    if (!resp)
        throw std::runtime_error("Failed to send request");
    std::string const& resp_body = resp.get_body();
    if (resp_body.length() < strlen("token=") || memcmp(resp_body.c_str(), "token=", strlen("token=")))
        throw std::runtime_error("Failed to obtain registration token");
    return resp_body.substr(strlen("token="));
}

std::string mcs_registration_api::obtain_instance_id_token(instance_id_request const& request) {
    registration_request req;
    req.app_pkg_name = request.app_pkg_name;
    req.app_cert = request.app_cert;
    req.app_ver = request.app_ver;
    req.senders.push_back(request.sender);
    if (!request.gmp_app_id.empty())
        req.extras["gmp_app_id"] = request.gmp_app_id;
    req.extras["subtype"] = req.extras["X-subtype"] = request.sender;
    req.extras["subscription"] = req.extras["X-subscription"] = request.sender;
    req.extras["appid"] = request.app_id;
    req.extras["scope"] = request.scope;
    std::string kid = "|ID|" + std::to_string(next_token_request_id++) + "|";
    req.extras["X-kid"] = req.extras["kid"] = kid;
    req.extras["osv"] = std::to_string(device.build_sdk_version);
    if (!request.pubkey.empty())
        req.extras["pub2"] = request.pubkey;
    if (!request.sig.empty())
        req.extras["sig"] = request.sig;
    req.extras["cliv"] = request.client_id;
    req.extras["gmsv"] = std::to_string(device.build_google_services);
    if (!request.app_ver.empty())
        req.extras["app_ver"] = request.app_ver;
    if (!request.app_ver_name.empty())
        req.extras["app_ver_name"] = request.app_ver_name;
    std::string token = perform_register(req);
    if (token.length() < kid.length() + 1 || memcmp(token.c_str(), kid.c_str(), kid.length()) ||
            token[kid.length()] != ':')
        throw std::runtime_error("Invalid token data");
    return token.substr(kid.length() + 1);
}