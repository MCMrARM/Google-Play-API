#include <iostream>
#include <pwd.h>
#include "login.h"

#include "httputils.h"
#include "device_info.h"

using namespace playdl;

void login_manager::perform_login(bool via_password, const std::string& email, const std::string& password,
                                  const std::string& master_token, bool is_access_token) {
    http_request req("https://android.clients.google.com/auth");
    req.set_user_agent("GoogleAuth/1.4 (" + device.build_product + " " + device.build_id + "); gzip");
    req.set_method(http_method::POST);
    url_encoded_entity ent;
    ent.add_pair("accountType", "HOSTED_OR_GOOGLE");
    if (via_password) {
        ent.add_pair("Email", email);
        ent.add_pair("Passwd", password);
    } else {
        ent.add_pair("Token", master_token);
        if (is_access_token)
            ent.add_pair("ACCESS_TOKEN", "1");
        if (email.length() <= 0) {
            ent.add_pair("add_account", "1");
        } else {
            ent.add_pair("Email", email);
            ent.add_pair("check_email", "1");
        }
    }
    if (device.android_id != 0) {
        ent.add_pair("androidId", device.get_string_android_id());
        req.add_header("device", device.get_string_android_id());
    }

    ent.add_pair("has_permission", "1");
    ent.add_pair("service", service);
    ent.add_pair("source", "android");
    ent.add_pair("app", app);
    req.add_header("app", app);
    ent.add_pair("device_country", device.country);
    ent.add_pair("lang", device.locale);
    ent.add_pair("sdk_version", std::to_string(device.build_sdk_version));
    if (cert == certificate::android)
        ent.add_pair("client_sig", "61ed377e85d386a8dfee6b864bd85b0bfaa5af81");
    else
        ent.add_pair("client_sig", "38918a453d07199354f8b19af05ec6562ced5788");
    ent.add_pair("system_partition", "1");
    req.set_body(ent);

    http_response resp = req.perform();
    const std::string& body = resp.get_body();
    std::map<std::string, std::string> respValMap;
    for (size_t i = 0; i < body.length();) {
        size_t j = body.find('=', i);
        size_t k = body.find('\n', j);
        if (j == std::string::npos || k == std::string::npos)
            break;
        std::string key = body.substr(i, j - i);
        std::string val = body.substr(j + 1, k - (body[k - 1] == '\r' ? 1 : 0) - (j + 1));
        respValMap[key] = val;
        i = k + 1;
    }
    if (respValMap.count("Error") > 0)
        throw std::runtime_error("Login error: " + respValMap.at("Error"));
    if (respValMap.count("Auth") <= 0)
        throw std::runtime_error("No auth cookie field returned");
    set_auth_cookie(respValMap.at("Auth"));
    if (via_password || is_access_token) {
        if (respValMap.count("Token") <= 0)
            throw std::runtime_error("No Oauth2 token returned");
        set_token(email, respValMap.at("Token"));
    }
    if (is_access_token) {
        if (respValMap.count("Email") <= 0)
            throw std::runtime_error("No Email returned");
        this->email = respValMap.at("Email");
    }
}

void login_manager::perform_login_using_password(const std::string& email, const std::string& password) {
    return perform_login(true, email, password, std::string(), false);
}

void login_manager::perform_login_using_access_token(const std::string& access_token) {
    return perform_login(true, std::string(), std::string(), access_token, true);
}

void login_manager::perform_login() {
    if (!has_token())
        throw std::logic_error("No token given.");
    return perform_login(false, email, std::string(), token, false);
}