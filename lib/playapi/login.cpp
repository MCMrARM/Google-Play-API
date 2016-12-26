#include <iostream>
#include <pwd.h>
#include "login.h"

#include "util/http.h"
#include "device_info.h"
#include "checkin.h"

using namespace playapi;

std::string login_api::perform(const login_request& request) {
    http_request req("https://android.clients.google.com/auth");
    req.set_user_agent("GoogleAuth/1.4 (" + device.build_product + " " + device.build_id + "); gzip");
    req.set_method(http_method::POST);
    url_encoded_entity ent;
    ent.add_pair("accountType", "HOSTED_OR_GOOGLE");
    if (request.via_password) {
        ent.add_pair("Email", request.email);
        ent.add_pair("Passwd", request.password);
    } else {
        ent.add_pair("Token", request.token);
        if (request.is_access_token)
            ent.add_pair("ACCESS_TOKEN", "1");
        if (email.length() <= 0) {
            ent.add_pair("add_account", "1");
        } else {
            ent.add_pair("Email", request.email);
            ent.add_pair("check_email", "1");
        }
    }
    if (android_id.length() > 0) {
        ent.add_pair("androidId", android_id);
        req.add_header("device", android_id);
    }

    ent.add_pair("has_permission", "1");
    ent.add_pair("service", request.service);
    ent.add_pair("source", "android");
    ent.add_pair("app", request.app);
    req.add_header("app", request.app);
    ent.add_pair("device_country", device.country);
    ent.add_pair("lang", device.locale);
    ent.add_pair("sdk_version", std::to_string(device.build_sdk_version));
    if (request.cert == certificate::android)
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
    auth_cookies[{request.service, request.app}] = respValMap.at("Auth");
    if (request.via_password || request.is_access_token) {
        if (respValMap.count("Token") <= 0)
            throw std::runtime_error("No Oauth2 token returned");
        set_token(request.email, respValMap.at("Token"));
    }
    if (request.is_access_token) {
        if (respValMap.count("Email") <= 0)
            throw std::runtime_error("No Email returned");
        this->email = respValMap.at("Email");
    }
    return respValMap.at("Auth");
}

void login_api::perform(const std::string& email, const std::string& password) {
    perform(login_request("ac2dm", "com.google.android.gsf", email, password));
}

void login_api::perform(const std::string& access_token) {
    perform(login_request("ac2dm", "com.google.android.gsf", access_token, true));
}

void login_api::verify() {
    perform(login_request("ac2dm", "com.google.android.gsf", token, false));
}

std::string login_api::fetch_service_auth_cookie(const std::string& service, const std::string& app, certificate cert) {
    if (token.size() == 0)
        throw std::runtime_error("No user authenticated.");
    if (auth_cookies.count({service, app}) > 0) {
        auto& cookie = auth_cookies.at({service, app});

    }
    return perform(login_request(service, app, token, false, cert));
}

void login_api::set_checkin_data(const checkin_result& result) {
    if (result.android_id != 0)
        android_id = result.get_string_android_id();
}