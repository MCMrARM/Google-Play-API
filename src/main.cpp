#include <iostream>
#include <fstream>
#include <curl/curl.h>
#include <playapi/login.h>
#include <playapi/device_info.h>
#include <playapi/util/config.h>
#include <playapi/checkin.h>
#include <playapi/api.h>
#include <gsf.pb.h>
#include <zlib.h>
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

static void do_zlib_inflate(z_stream& zs, FILE* file, char* data, size_t len, int flags) {
    char buf[4096];
    int ret;
    zs.avail_in = (uInt) len;
    zs.next_in = (unsigned char*) data;
    zs.avail_out = 0;
    while (zs.avail_out == 0) {
        zs.avail_out = 4096;
        zs.next_out = (unsigned char*) buf;
        ret = inflate(&zs, flags);
        assert(ret != Z_STREAM_ERROR);
        fwrite(buf, 1, sizeof(buf) - zs.avail_out, file);
    }
}

int main() {
    curl_global_init(CURL_GLOBAL_ALL);
    GOOGLE_PROTOBUF_VERIFY_VERSION;

    conf.load();

    device_info device;

    std::string device_path = "devices/default.conf";

    {
        std::ifstream dev_info_file(device_path);
        config dev_info_conf;
        dev_info_conf.load(dev_info_file);
        device.load(dev_info_conf);
    }

    device_config dev_state(device_path + ".state");
    dev_state.load();
    dev_state.load_device_info_data(device);
    device.generate_fields();
    dev_state.set_device_info_data(device);
    dev_state.save();

    login_api login(device);
    login.set_checkin_data(dev_state.checkin_data);
    if (conf.user_token.length() <= 0) {
        do_interactive_auth(login);
    } else {
        do_auth_from_config(login);
    }
    if (dev_state.checkin_data.android_id == 0) {
        checkin_api checkin(device);
        checkin.add_auth(login);
        dev_state.checkin_data = checkin.perform_checkin();
        dev_state.save();
    }

    api play(device);
    play.set_auth(login);
    play.set_checkin_data(dev_state.checkin_data);
    dev_state.load_api_data(login.get_email(), play);
    if (play.toc_cookie.length() == 0 || play.device_config_token.length() == 0) {
        play.fetch_user_settings();
        auto toc = play.fetch_toc();
        if (toc.payload().tocresponse().has_cookie())
            play.toc_cookie = toc.payload().tocresponse().cookie();

        if (play.fetch_toc().payload().tocresponse().requiresuploaddeviceconfig()) {
            auto resp = play.upload_device_config();
            play.device_config_token = resp.payload().uploaddeviceconfigresponse().uploaddeviceconfigtoken();

            toc = play.fetch_toc();
            assert(!toc.payload().tocresponse().requiresuploaddeviceconfig() &&
                   toc.payload().tocresponse().has_cookie());
            play.toc_cookie = toc.payload().tocresponse().cookie();
            if (toc.payload().tocresponse().has_toscontent() && toc.payload().tocresponse().has_tostoken()) {
                std::string str;
                std::cout << "Terms of Service:\n" << toc.payload().tocresponse().toscontent() << " [y/N]: ";
                std::getline(std::cin, str);
                if (str[0] != 'Y' && str[0] != 'y') {
                    std::cout << "You have to accept the Terms of Service!\n";
                    return 1;
                }
                std::cout << "Optional: " << toc.payload().tocresponse().toscheckboxtextmarketingemails() << " [y/N]: ";
                std::getline(std::cin, str);
                auto tos = play.accept_tos(toc.payload().tocresponse().tostoken(), str[0] == 'Y' || str[0] == 'y');
                assert(tos.payload().has_accepttosresponse());
                dev_state.set_api_data(login.get_email(), play);
                dev_state.save();
            }
        }
    }

    std::cout << "Please type the name of the app you want to download: ";
    std::string pkg_name;
    std::cin >> pkg_name;
    auto details = play.details(pkg_name).payload().detailsresponse().docv2();
    if (!details.details().appdetails().has_versioncode()) {
        std::cout << "No version code found. Did you specify a valid package name?\n";
        return 1;
    }

    // TODO: Free app purchase flow

    {
        // TODO: we should pass a valid library token here - however that requires implementation of the
        // replicateLibrary API call
        auto resp = play.delivery(pkg_name, details.details().appdetails().versioncode(), std::string());
        auto dd = resp.payload().deliveryresponse().appdeliverydata();
        http_request req(dd.gzippeddownloadurl());
        req.set_encoding("gzip,deflate");
        req.add_header("Accept-Encoding", "identity");
        auto cookie = dd.downloadauthcookie(0);
        req.add_header("Cookie", cookie.name() + "=" + cookie.value());
        req.set_user_agent("AndroidDownloadManager/" + device.build_version_string + " (Linux; U; Android " +
                           device.build_version_string + "; " + device.build_model + " Build/" + device.build_id + ")");
        req.set_follow_location(true);
        req.set_timeout(0L);

        FILE* file = fopen(
                (pkg_name + " " + std::to_string(details.details().appdetails().versioncode()) + ".apk").c_str(), "w");
        z_stream zs;
        zs.zalloc = Z_NULL;
        zs.zfree = Z_NULL;
        zs.opaque = Z_NULL;
        int ret = inflateInit2(&zs, 31);
        assert(ret == Z_OK);

        req.set_custom_output_func([file, &zs](char* data, size_t size) {
            do_zlib_inflate(zs, file, data, size, Z_NO_FLUSH);
            return size;
        });

        req.set_progress_callback([&req](curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow) {
            if (dltotal > 0) {
                printf("\rDownloaded %i%% [%li/%li MiB]", (int) (dlnow * 100 / dltotal), dlnow / 1024 / 1024,
                       dltotal / 1024 / 1024);
                std::cout.flush();
            }
        });
        std::cout << "\nStarting download...";
        req.perform();

        do_zlib_inflate(zs, file, Z_NULL, 0, Z_FINISH);
        inflateEnd(&zs);

        fclose(file);
    }

    curl_global_cleanup();
    google::protobuf::ShutdownProtobufLibrary();
    return 0;
}