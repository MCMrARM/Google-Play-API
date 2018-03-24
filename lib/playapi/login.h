#pragma once

#include <string>
#include <map>
#include "util/config.h"

namespace playapi {

class device_info;
struct checkin_result;

class login_api {

public:

    enum class certificate {
        android, google
    };

private:

    static std::regex entry_parse_regex;

    struct login_request {
        std::string service, app;
        bool via_password = false;
        std::string email, password;
        std::string token;
        bool is_access_token = false;
        bool is_add_account = false;
        certificate cert = certificate::google;

        login_request(const std::string& service, const std::string& app, std::string email, std::string password,
                      certificate cert = certificate::google) : service(service), app(app), via_password(true),
                                                                email(email), password(password), cert(cert) {}

        login_request(const std::string& service, const std::string& app, const std::string& email,
                      const std::string& token, bool is_access_token, bool is_add_account,
                      certificate cert = certificate::google) : service(service), app(app), email(email), token(token),
                                                                is_access_token(is_access_token),
                                                                is_add_account(is_access_token), cert(cert) {}
    };

    const device_info& device;

    std::string email, token;
    std::string android_id;

    std::map<std::pair<std::string, std::string>, std::string> auth_cookies;

    std::string perform(const login_request& request);

public:

    login_api(const device_info& device) : device(device) {
        //
    }

    void perform(const std::string& email, const std::string& password);

    // this function will perform login using the specified access token
    // this token is NOT the same as the token that you can provide to set_token
    void perform_with_access_token(const std::string& access_token, const std::string& email = std::string(),
                                   bool add_account = false);

    void verify();


    std::string fetch_service_auth_cookie(const std::string& service, const std::string& app, certificate cert);


    void set_checkin_data(const checkin_result& result);

    bool has_token() const {
        return token.length() > 0;
    }

    std::string const& get_token() const {
        return token;
    }

    std::string const& get_email() const {
        return email;
    }

    void set_token(std::string const& email, std::string const& token) {
        this->email = email;
        this->token = token;
    }


    void load_auth_cookies(std::vector<std::string> const& v);

    std::vector<std::string> store_auth_cookies();

};

}