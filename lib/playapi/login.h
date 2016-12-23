#pragma once

#include <string>
#include <map>

namespace playapi {

class device_info;

class login_api {

public:

    enum class certificate {
        android, google
    };

private:

    struct login_request {
        std::string service, app;
        bool via_password = false;
        std::string email, password;
        std::string token;
        bool is_access_token = false;
        certificate cert = certificate::google;

        login_request(const std::string& service, const std::string& app, std::string email, std::string password,
                      certificate cert = certificate::google) : service(service), app(app), via_password(true),
                                                                email(email), password(password), cert(cert) {}
        login_request(const std::string& service, const std::string& app, const std::string& token,
                      bool is_access_token, certificate cert = certificate::google) : service(service), app(app),
                                                                                      token(token),
                                                                                      is_access_token(is_access_token),
                                                                                      cert(cert) {}
    };

    const device_info& device;

    std::string email, token;

    std::map<std::pair<std::string, std::string>, std::string> auth_cookies;

    std::string perform(const login_request& request);

public:

    login_api(const device_info& device) : device(device) {
        //
    }

    void perform(const std::string& email, const std::string& password);

    // this function will perform login using the specified access token
    // this token is NOT the same as the token that you can provide to set_token
    void perform(const std::string& access_token);

    void verify();


    std::string fetch_service_auth_cookie(const std::string& service, const std::string& app, certificate cert);


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

};

}