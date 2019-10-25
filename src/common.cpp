#include <iostream>
#include <fstream>
#include <curl/curl.h>
#include "../include/playapi/login.h"
#include "../include/playapi/device_info.h"
#include "../include/playapi/util/config.h"
#include "../include/playapi/checkin.h"
#include "../include/playapi/api.h"
#include <gsf.pb.h>
#include <zlib.h>
#include "../include/playapi/file_login_cache.h"
#include "config.h"
#include "common.h"

using namespace playapi;

playapi_cli_base::playapi_cli_base() : conf("playdl.conf"), login_cache("token_cache.conf"),
    login_api(device, login_cache), api(device) {

}

void playapi_cli_base::do_interactive_auth() {
    auth_select_method:
    std::cout << "We haven't authorized you yet. Please select your preferred login method:" << std::endl;
    std::cout << "1. via login and password" << std::endl;
    std::cout << "2. via access token" << std::endl;
    std::cout << "3. via master token" << std::endl;
    std::cout << "Please type the selected number and press enter: ";
    std::string method;
    std::getline(std::cin, method);
    std::cout << std::endl;
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
            login_api.perform(email, password)->call();
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
            login_api.perform_with_access_token(token)->call();
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
        login_api.set_token("", token);
        try {
            login_api.verify()->call();
        } catch (std::runtime_error err) {
            std::cout << "Failed to login using the specified token: " << err.what() << std::endl << std::endl;
            goto do_master_token_auth;
        }
        std::cout << std::endl;
    } else {
        std::cout << "You have entered an invalid number." << std::endl << std::endl;
        goto auth_select_method;
    }

    if (login_api.has_token()) {
        std::cout << "We can save a token on your hard drive that will allow you to use this application"
                  << " without having to sign in again." << std::endl;
        std::cout << "Would you like to store the token? [Y/n]: ";
        std::string answ;
        std::getline(std::cin, answ);
        if (answ.length() == 0 || answ[0] == 'Y' || answ[0] == 'y') {
            conf.user_email = login_api.get_email();
            conf.user_token = login_api.get_token();
            conf.save();
            std::cout << "Saved the token." << std::endl;
        }
    } else {
        std::cout << "Something went wrong. Let's try again." << std::endl;
        goto auth_select_method;
    }
}

void playapi_cli_base::do_auth_from_config() {
    do_auth:
    login_api.set_token(conf.user_email, conf.user_token);
    if (opt_login_no_verify)
        return;

    try {
        login_api.verify()->call();
    } catch (std::runtime_error &err) {
        if (!opt_interactive) {
            std::cerr << "error: bad saved token" << std::endl;
            exit(1);
        }

        std::cout << "Failed to login using a saved token: " << err.what() << std::endl;
        std::cout << "Would you like to delete it and authenticate again? [y/N]";
        std::string answ;
        std::getline(std::cin, answ);
        if (answ[0] == 'Y' || answ[0] == 'y') {
            conf.user_email = "";
            conf.user_token = "";
            do_interactive_auth();
            return;
        }
        goto do_auth;
    }
}

void playapi_cli_base::print_global_help() {
    std::cout << "Global options" << std::endl;
    std::cout << "-i   --interactive      Enables interactive mode" << std::endl;
    std::cout << "-u   --email            Email to use for automatic login" << std::endl;
    std::cout << "-p   --password         Password to use for automatic login" << std::endl;
    std::cout << "-t   --token            Token to use for automatic login" << std::endl;
    std::cout << "-sa  --save-auth        Save authentication information to file" << std::endl;
    std::cout << "-tos --accept-tos       Automatically accept ToS if needed" << std::endl;
    std::cout << "-d   --device           Use the specified device configuration file" << std::endl;
    std::cout << "-nv  --login-no-verify  Disable login credential verification" << std::endl << std::endl;
}

bool playapi_cli_base::parse_arg(arg_list &list, const char *key) {
    if (strcmp(key, "-i") == 0 || strcmp(key, "--interactive") == 0)
        opt_interactive = true;
    else if (strcmp(key, "-u") == 0 || strcmp(key, "--email") == 0)
        opt_email = list.next_value();
    else if (strcmp(key, "-p") == 0 || strcmp(key, "--password") == 0)
        opt_password = list.next_value();
    else if (strcmp(key, "-t") == 0 || strcmp(key, "--token") == 0)
        opt_token = list.next_value();
    else if (strcmp(key, "-sa") == 0 || strcmp(key, "--save-auth") == 0)
        opt_save_auth = true;
    else if (strcmp(key, "-tos") == 0 || strcmp(key, "--accept-tos") == 0)
        opt_accept_tos = true;
    else if (strcmp(key, "-d") == 0 || strcmp(key, "--device") == 0)
        opt_device_path = list.next_value();
    else if (strcmp(key, "-nv") == 0 || strcmp(key, "--login-no-verify") == 0)
        opt_login_no_verify = true;
    else
        return false;
    return true;
}

void playapi_cli_base::parse_args(int argc, const char **argv) {
    arg_list list (argc, argv);
    list.next();
    if ((list.peek()) == nullptr) {
        opt_interactive = true; // with no args, default to interactive
        return;
    }
    const char *key;
    while ((key = list.next_or_null())) {
        if (strcmp(key, "-h") == 0 || strcmp(key, "--help") == 0) {
            print_help();
            exit(1);
        }

        bool b = parse_arg(list, key);
        if (!b)
            std::cerr << "error: bad argument: " << key << std::endl;
    }
}

void playapi_cli_base::perform_auth() {
    {
        std::ifstream dev_info_file(opt_device_path);
        config dev_info_conf;
        dev_info_conf.load(dev_info_file);
        device.load(dev_info_conf);
    }

    device_config dev_state(opt_device_path + ".state");
    dev_state.load();
    dev_state.load_device_info_data(device);
    device.generate_fields();
    dev_state.set_device_info_data(device);
    dev_state.save();

    login_api.set_checkin_data(dev_state.checkin_data);
    if (opt_token.length() > 0) {
        login_api.set_token(opt_email, opt_token);
        if (!opt_login_no_verify) {
            try {
                login_api.verify()->call();
            } catch (std::runtime_error &err) {
                std::cerr << "error: bad token" << std::endl;
                exit(1);
            }
        }
    } else if (conf.user_token.length() <= 0) {
        if (opt_email.length() > 0 && opt_password.length() > 0) {
            login_api.perform(opt_email, opt_password);
            if (opt_save_auth) {
                conf.user_email = login_api.get_email();
                conf.user_token = login_api.get_token();
                conf.save();
            }
        } else if (opt_interactive) {
            do_interactive_auth();
        } else {
            std::cerr << "error: no authentication set and no playdl.conf file exists" << std::endl;
            exit(1);
        }
    } else {
        do_auth_from_config();
    }
    if (dev_state.checkin_data.android_id == 0) {
        checkin_api checkin(device);
        checkin.add_auth(login_api)->call();
        dev_state.checkin_data = checkin.perform_checkin()->call();
        dev_state.save();
    }

    api.set_auth(login_api)->call();
    api.set_checkin_data(dev_state.checkin_data);
    dev_state.load_api_data(login_api.get_email(), api);
    if (api.toc_cookie.length() == 0 || api.device_config_token.length() == 0) {
        api.fetch_user_settings()->call();
        auto toc = api.fetch_toc()->call();
        if (toc.payload().tocresponse().has_cookie())
            api.toc_cookie = toc.payload().tocresponse().cookie();

        if (api.fetch_toc()->call().payload().tocresponse().requiresuploaddeviceconfig()) {
            auto resp = api.upload_device_config()->call();
            api.device_config_token = resp.payload().uploaddeviceconfigresponse().uploaddeviceconfigtoken();

            toc = api.fetch_toc()->call();
            assert(!toc.payload().tocresponse().requiresuploaddeviceconfig() &&
                   toc.payload().tocresponse().has_cookie());
            api.toc_cookie = toc.payload().tocresponse().cookie();
            if (toc.payload().tocresponse().has_toscontent() && toc.payload().tocresponse().has_tostoken()) {
                if (opt_interactive)
                    std::cout << "Terms of Service:" << std::endl
                              << toc.payload().tocresponse().toscontent() << " [y/N]: ";
                bool allow_marketing_emails = false;
                if (!opt_accept_tos) {
                    std::string str;
                    if (!opt_interactive) {
                        std::cerr << "error: tos not accepted" << std::endl;
                        exit(1);
                    }
                    std::getline(std::cin, str);
                    if (str[0] != 'Y' && str[0] != 'y') {
                        std::cout << "You have to accept the Terms of Service!" << std::endl;
                        exit(1);
                    }
                    std::cout << "Optional: " << toc.payload().tocresponse().toscheckboxtextmarketingemails()
                              << " [y/N]: ";
                    std::getline(std::cin, str);
                    allow_marketing_emails = (str[0] == 'Y' || str[0] == 'y');
                }
                auto tos = api.accept_tos(toc.payload().tocresponse().tostoken(), allow_marketing_emails)->call();
                assert(tos.payload().has_accepttosresponse());
                dev_state.set_api_data(login_api.get_email(), api);
                dev_state.save();
            }
        }
    }
}
