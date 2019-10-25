#pragma once

#include "arg_list.h"
#include <playapi/device_info.h>
#include <playapi/login.h>
#include <playapi/api.h>

class playapi_cli_base {

public:
    bool opt_interactive = false;
    std::string opt_email, opt_password, opt_token;
    bool opt_save_auth = false, opt_login_no_verify = false;
    bool opt_accept_tos = false;
    std::string opt_device_path = "devices/default.conf";

    app_config conf;
    playapi::device_info device;
    playapi::file_login_cache login_cache;
    playapi::login_api login_api;
    playapi::api api;

    playapi_cli_base();


    void print_global_help();

    virtual void print_help() = 0;

    virtual bool parse_arg(arg_list &list, const char *key);

    void parse_args(int argc, const char* argv[]);

    void perform_auth();

private:
    void do_interactive_auth();

    void do_auth_from_config();

};