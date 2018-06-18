#pragma once

#include <string>
#include <map>
#include "login_cache.h"
#include "http_task.h"

namespace playapi {

class device_info;
struct checkin_result;

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

    std::mutex mutex;
    std::string email, token;
    std::string android_id;

    login_cache& cache;

    task_ptr<std::string> perform(const login_request& request);

    std::string handle_response(http_response& resp, login_request const& request, std::chrono::system_clock::time_point start);

public:

    login_api(const device_info& device, login_cache& cache) : device(device), cache(cache) {
    }

    task_ptr<void> perform(const std::string& email, const std::string& password);

    // this function will perform login using the specified access token
    // this token is NOT the same as the token that you can provide to set_token
    task_ptr<void> perform_with_access_token(const std::string& access_token, const std::string& email = std::string(),
                                              bool add_account = false);

    task_ptr<void> verify();


    task_ptr<std::string> fetch_service_auth_cookie(const std::string& service, const std::string& app,
                                                    certificate cert, bool force_refresh = false);


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

};

}