#pragma once

#include <string>

namespace playdl {

class device_info;

class login_manager {

public:

    enum class certificate {
        android, google
    };

private:

    std::string service, app;
    certificate cert;
    const device_info& device;

    std::string email, token, auth_cookie;

    void perform_login(bool via_password, const std::string& email, const std::string& password,
                       const std::string& master_token, bool is_access_token);

public:

    login_manager(const std::string& service, const std::string& app, const device_info& device, certificate cert) :
            service(service), app(app), device(device), cert(cert) {
        //
    }

    void perform_login_using_password(const std::string& email, const std::string& password);

    // this function will perform login using the specified access token
    // this token is NOT the same as the token that you can provide to set_token
    void perform_login_using_access_token(const std::string& access_token);

    // this function requires that you specify the token via set_token before
    void perform_login();


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


    bool has_auth_cookie() const {
        return auth_cookie.size() > 0;
    }

    std::string const& get_auth_cookie() const {
        return auth_cookie;
    }

    void set_auth_cookie(std::string const& auth_cookie) {
        this->auth_cookie = auth_cookie;
    }

};

}