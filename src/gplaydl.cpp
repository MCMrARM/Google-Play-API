#include <iostream>
#include <curl/curl.h>
#include <gsf.pb.h>
#include <zlib.h>
#include "../include/playapi/file_login_cache.h"
#include "config.h"
#include "common.h"

using namespace playapi;

struct playapi_cli_downloader : playapi_cli_base {

    std::string opt_app;
    int opt_app_version = -1;
    std::string opt_app_output;

    bool parse_arg(arg_list &list, const char *key) override;

    void print_help() override {
        std::cout << "Google Play Downloader tool" << std::endl << std::endl;
        print_global_help();
        std::cout << "Download options:" << std::endl;
        std::cout << "-a   --app              Download this app (package name)" << std::endl;
        std::cout << "-v   --app-version      Specify the version to download" << std::endl;
        std::cout << "-o   --output           Specify the output file name" << std::endl;;
    }

    void download_file(http_request &req, std::string file_name, bool gzipped);

    void run();

};

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

bool playapi_cli_downloader::parse_arg(arg_list &list, const char *key) {
    if (strcmp(key, "-a") == 0 || strcmp(key, "--app") == 0)
        opt_app = list.next_value();
    else if (strcmp(key, "-v") == 0 || strcmp(key, "--app-version") == 0)
        opt_app_version = atoi(list.next_value());
    else if (strcmp(key, "-o") == 0 || strcmp(key, "--output") == 0)
        opt_app_output = list.next_value();
    else
        return playapi_cli_base::parse_arg(list, key);
    return true;
}

void playapi_cli_downloader::run() {
    if (opt_app.length() == 0) {
        if (!opt_interactive) {
            std::cerr << "error: no app package name provided\n";
            exit(1);
        }
        std::cout << "Please type the name of the app you want to download: ";
        std::cin >> opt_app;
    }
    if (opt_app_version == -1) {
        auto details = api.details(opt_app)->call().payload().detailsresponse().docv2();
        if (!details.details().appdetails().has_versioncode()) {
            if (opt_interactive)
                std::cerr << "No version code found. Did you specify a valid package name?" << std::endl;
            else
                std::cerr << "error: no version code found\n";
            exit(1);
        }
        opt_app_version = details.details().appdetails().versioncode();
    }

    // TODO: Free app purchase flow

    {
        // TODO: we should pass a valid library token here - however that requires implementation of the
        // replicateLibrary API call
        auto resp = api.delivery(opt_app, opt_app_version, std::string())->call();
        auto dd = resp.payload().deliveryresponse().appdeliverydata();
        http_request req(dd.has_gzippeddownloadurl() ? dd.gzippeddownloadurl() : dd.downloadurl());
        req.add_header("Accept-Encoding", "identity");
        auto cookie = dd.downloadauthcookie(0);
        req.add_header("Cookie", cookie.name() + "=" + cookie.value());
        req.set_user_agent("AndroidDownloadManager/" + device.build_version_string + " (Linux; U; Android " +
                           device.build_version_string + "; " + device.build_model + " Build/" + device.build_id + ")");
        req.set_follow_location(true);

        std::string file_name = opt_app_output;
        if (file_name.length() == 0)
            file_name = opt_app + " " + std::to_string(opt_app_version) + ".apk";

        download_file(req, file_name, dd.has_gzippeddownloadurl());
    }
}

void playapi_cli_downloader::download_file(http_request &req, std::string file_name, bool gzipped) {
    if (gzipped)
        req.set_encoding("gzip,deflate");
    req.set_timeout(0L);

    FILE* file = fopen(file_name.c_str(), "w");
    z_stream zs;
    zs.zalloc = Z_NULL;
    zs.zfree = Z_NULL;
    zs.opaque = Z_NULL;

    if (gzipped) {
        int ret = inflateInit2(&zs, 31);
        assert(ret == Z_OK);

        req.set_custom_output_func([file, &zs](char* data, size_t size) {
            do_zlib_inflate(zs, file, data, size, Z_NO_FLUSH);
            return size;
        });
    } else {
        req.set_custom_output_func([file, &zs](char* data, size_t size) {
            fwrite(data, sizeof(char), size, file);
            return size;
        });
    }

    req.set_progress_callback([&req](curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow) {
        if (dltotal > 0) {
            printf("\rDownloaded %i%% [%lli/%lli MiB]", (int) (dlnow * 100 / dltotal), dlnow / 1024 / 1024,
                   dltotal / 1024 / 1024);
            std::cout.flush();
        }
    });
    std::cout << std::endl << "Starting download...";
    req.perform();

    if (gzipped) {
        do_zlib_inflate(zs, file, Z_NULL, 0, Z_FINISH);
        inflateEnd(&zs);
    }

    fclose(file);
}

int main(int argc, char* argv[]) {
    curl_global_init(CURL_GLOBAL_ALL);
    GOOGLE_PROTOBUF_VERIFY_VERSION;

    playapi_cli_downloader cli;
    cli.parse_args(argc, (const char **) argv);
    cli.perform_auth();
    cli.run();

    curl_global_cleanup();
    google::protobuf::ShutdownProtobufLibrary();
    return 0;
}
