#include <iostream>
#include <fstream>
#include <curl/curl.h>
#include <playapi/login.h>
#include <playapi/device_info.h>
#include <playapi/config.h>
#include <playapi/checkin.h>
#include <gsf.pb.h>
#include "config.h"

using namespace playapi;

app_config conf;

void do_interactive_auth(login_api& login) {
    auth_select_method:
    std::cout << "We haven't authorized you yet. Please select your preferred login method:" << std::endl;
    std::cout << "1. via login and password" << std::endl;
    std::cout << "2. via access token" << std::endl;
    std::cout << "3. via master token" << std::endl;
    std::cout << "Please type the selected number and press enter: ";
    std::string method;
    std::getline(std::cin, method);
    std::cout << "\n";
    if (method == "1") {
        // via login & password
        do_login_pass_auth:
        std::cout << "Please enter your email: ";
        std::string email;
        std::getline(std::cin, email);
        std::cout << "Please enter password for your account: ";
        std::string password;
        std::getline(std::cin, password);
        std::cout << "Authenticating..." << std::endl;
        try {
            login.perform(email, password);
        } catch (std::runtime_error err) {
            std::cout << "Failed to login using the specified credentials: " << err.what() << std::endl;
            goto do_login_pass_auth;
        }
        std::cout << "Logged you in successfully." << std::endl;
        std::cout << std::endl;
    } else if (method == "2") {
        // via master token
        do_token_auth:
        std::cout << "Please enter your access token: ";
        std::string token;
        std::getline(std::cin, token);
        try {
            login.perform(token);
        } catch (std::runtime_error err) {
            std::cout << "Failed to login using the specified token: " << err.what() << std::endl << std::endl;
            goto do_token_auth;
        }
        std::cout << std::endl;
    } else if (method == "3") {
        // via master token
        do_master_token_auth:
        std::cout << "Please enter your master token: ";
        std::string token;
        std::getline(std::cin, token);
        login.set_token("", token);
        try {
            login.verify();
        } catch (std::runtime_error err) {
            std::cout << "Failed to login using the specified token: " << err.what() << std::endl << std::endl;
            goto do_master_token_auth;
        }
        std::cout << std::endl;
    } else {
        std::cout << "You have entered an invalid number." << std::endl << std::endl;
        goto auth_select_method;
    }

    if (login.has_token()) {
        std::cout << "We can save a token on your hard drive that will allow you to use this application"
                  << " without having to sign in again." << std::endl;
        std::cout << "Would you like to store the token? [Y/n]: ";
        std::string answ;
        std::getline(std::cin, answ);
        if (answ.length() == 0 || answ[0] == 'Y' || answ[0] == 'y') {
            conf.user_email = login.get_email();
            conf.user_token = login.get_token();
            conf.save();
            std::cout << "Saved the token." << std::endl;
        }
    } else {
        std::cout << "Something went wrong. Let's try again." << std::endl;
        goto auth_select_method;
    }
}
void do_auth_from_config(login_api& login) {
    do_auth:
    login.set_token(conf.user_email, conf.user_token);
    try {
        login.verify();
    } catch (std::runtime_error err) {
        std::cout << "Failed to login using a saved token: " << err.what() << std::endl;
        std::cout << "Would you like to delete it and authenticate again? [y/N]";
        std::string answ;
        std::getline(std::cin, answ);
        if (answ[0] == 'Y' || answ[0] == 'y') {
            conf.user_email = "";
            conf.user_token = "";
            do_interactive_auth(login);
            return;
        }
        goto do_auth;
    }
}

int main() {
    curl_global_init(CURL_GLOBAL_ALL);
    GOOGLE_PROTOBUF_VERIFY_VERSION;

    conf.load();

    device_info device;

    std::string device_path = "devices/default.conf";

    std::ifstream device_conf_file (device_path);
    config device_conf;
    device_conf.load(device_conf_file);
    device.load(device_conf);
    conf.load_device(device_path, device);
    device.generate_fields();
    conf.save_device(device_path, device);

    login_api login (device);
    if (conf.user_token.length() <= 0) {
        do_interactive_auth(login);
    } else {
        do_auth_from_config(login);
    }
    if (device.android_id == 0) {
        checkin_api checkin (device);
        checkin.add_auth(login);
        checkin.perform();
        conf.save_device(device_path, device);
    }

    curl_global_cleanup();
    google::protobuf::ShutdownProtobufLibrary();
    return 0;
}