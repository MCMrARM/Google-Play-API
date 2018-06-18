#include <iostream>
#include <pwd.h>
#include <cstring>
#include <playapi/login.h>
#include <playapi/util/http.h>
#include <playapi/device_info.h>
#include <playapi/checkin.h>

using namespace playapi;

task_ptr<std::string> login_api::perform(const login_request& request) {
    auto start = std::chrono::system_clock::now();
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
        if (!request.email.empty())
            ent.add_pair("Email", request.email);
        if (request.is_add_account) {
            ent.add_pair("add_account", "1");
        } else if (!request.email.empty()) {
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

    return http_task::make(req)->then<std::string>([this, request, start](http_response&& res) {
        return handle_response(res, request, start);
    });
}

std::string login_api::handle_response(http_response& resp, login_request const& request,
                                       std::chrono::system_clock::time_point start) {
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
    auto auth_val = respValMap.at("Auth");
    auto expires = start;
    if (respValMap.count("Expires") > 0) {
        expires += std::chrono::seconds(std::stoi(respValMap.at("Expires")));
    } else {
        if (strncmp(auth_val.c_str(), "oauth2:", 7) == 0)
            expires += std::chrono::hours(1);
        else
            expires += std::chrono::hours(24 * 14);
    }
    cache.cache(request.service, request.app, auth_val, expires);
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
    return auth_val;
}

task_ptr<void> login_api::perform(const std::string& email, const std::string& password) {
    return perform(login_request("ac2dm", "com.google.android.gsf", email, password))->then<void>([](std::string&&){});
}

task_ptr<void> login_api::perform_with_access_token(const std::string& access_token, const std::string& email,
                                                    bool add_account) {
    return perform(login_request("ac2dm", "com.google.android.gsf", email, access_token, true, add_account))->
            then<void>([](std::string&&){});
}

task_ptr<void> login_api::verify() {
    return perform(login_request("ac2dm", "com.google.android.gsf", std::string(), token, false, false))->
            then<void>([](std::string&&){});
}

task_ptr<std::string> login_api::fetch_service_auth_cookie(const std::string& service, const std::string& app,
                                                           certificate cert, bool force_refresh) {
    if (token.empty())
        throw std::runtime_error("No user authenticated.");
    auto cookie = cache.get_cached(service, app);
    if (!cookie.empty() && !force_refresh)
        return pre_set_task<std::string>::make(cookie);
    return perform(login_request(service, app, email, token, false, false, cert));
}

void login_api::set_checkin_data(const checkin_result& result) {
    if (result.android_id != 0)
        android_id = result.get_string_android_id();
}
