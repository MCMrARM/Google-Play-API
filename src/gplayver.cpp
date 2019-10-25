#include <iostream>
#include <curl/curl.h>
#include <gsf.pb.h>
#include <zlib.h>
#include "../include/playapi/file_login_cache.h"
#include "config.h"
#include "common.h"

using namespace playapi;

struct playapi_cli_ver : playapi_cli_base {

    std::string opt_app;
    bool opt_fast = false;

    bool parse_arg(arg_list &list, const char *key) override;

    void print_help() override {
        std::cout << "Google Play App Info tool" << std::endl << std::endl;
        print_global_help();
        std::cout << "App Info options:" << std::endl;
        std::cout << "-a   --app              Download this app (package name)" << std::endl;
        std::cout << "-f   --fast             Uses a more lightweight query" << std::endl;
    }

    void run();

};

bool playapi_cli_ver::parse_arg(arg_list &list, const char *key) {
    if (strcmp(key, "-a") == 0 || strcmp(key, "--app") == 0)
        opt_app = list.next_value();
    else if (strcmp(key, "-f") == 0 || strcmp(key, "--fast") == 0)
        opt_fast = true;
    else
        return playapi_cli_base::parse_arg(list, key);
    return true;
}

void playapi_cli_ver::run() {
    if (opt_fast) {
        auto details = api.bulk_details({opt_app})->call();
        if (details.payload().bulkdetailsresponse().entry_size() != 1)
            throw std::runtime_error("Invalid response: entry_size() != 1");
        if (!details.payload().bulkdetailsresponse().entry(0).has_doc() ||
            !details.payload().bulkdetailsresponse().entry(0).doc().has_details() ||
            !details.payload().bulkdetailsresponse().entry(0).doc().details().has_appdetails())
            throw std::runtime_error("Invalid response: does not have details");
        auto app_details = details.payload().bulkdetailsresponse().entry(0).doc().details().appdetails();
        std::cout << "version code: " << app_details.versioncode() << std::endl;
        std::cout << "version string: " << app_details.versionstring() << std::endl;
        std::cout << "changelog: " << app_details.recentchangeshtml() << std::endl;
        return;
    }

    auto details = api.details(opt_app)->call().payload().detailsresponse().docv2();
    std::cout << "version code: " << details.details().appdetails().versioncode() << std::endl;
    std::cout << "version string: " << details.details().appdetails().versionstring() << std::endl;
    std::cout << "changelog: " << details.details().appdetails().recentchangeshtml() << std::endl;
}

int main(int argc, char* argv[]) {
    curl_global_init(CURL_GLOBAL_ALL);
    GOOGLE_PROTOBUF_VERIFY_VERSION;

    playapi_cli_ver cli;
    cli.parse_args(argc, (const char **) argv);
    cli.perform_auth();
    cli.run();

    curl_global_cleanup();
    google::protobuf::ShutdownProtobufLibrary();
    return 0;
}
