#pragma once

#include <string>
#include <vector>
#include "util/http.h"
#include <play_respone.pb.h>
#include <play_settings.pb.h>
#include "experiments.h"
#include "checkin.h"

namespace playapi {

class device_info;
class login_api;

class api {

public:

    enum class request_content_type {
        none, url_encoded, protobuf
    };
    struct request_options {
        request_content_type content_type = request_content_type::none;
        bool include_checkin_consistency_token = true;
        bool include_content_filters = true;
        bool include_network_type = true;
        bool include_toc_cookie = true;
        bool include_device_config_token = false;
    };

private:

    device_info& device;
    login_api* login;
    std::string url;

    std::mutex auth_mutex;
    std::string auth_email, auth_token;
    checkin_result checkin_data;

    std::string build_user_agent();

    void add_headers(http_request& req, const request_options& options);

    task_ptr<void> invalidate_token();

public:

    using request_task = task_ptr<proto::finsky::response::ResponseWrapper>;

    mutable std::mutex info_mutex;
    std::string device_config_token;
    std::string toc_cookie;
    experiments_list experiments;


    api(device_info& device, const std::string& url = "https://android.clients.google.com/fdfe/") : device(device),
                                                                                                    url(url) {}

    task_ptr<void> set_auth(login_api& login);

    void set_checkin_data(const checkin_result& result);

    request_task
    send_request(http_method method, const std::string& path, const request_options& options);

    request_task
    send_request(http_method method, const std::string& path, const std::string& bin_data,
                 const request_options& options);

    request_task
    send_request(http_method method, const std::string& path,
                 const std::vector<std::pair<std::string, std::string>>& pairs, const request_options& options);

    request_task fetch_user_settings();

    request_task fetch_toc();

    request_task upload_device_config(const std::string& gcm_reg_id = "", bool upload_full_config = true);

    request_task accept_tos(const std::string& token, bool allow_marketing_emails = false);

    request_task get_search_suggestions(std::string q, int backend_id = 3, int icon_size = 120, bool request_navigational = true);

    request_task search(const std::string& q, int backend_id = 3);

    request_task details(const std::string& app);

    request_task
    delivery(const std::string& app, int version_code, const std::string& library_token,
             const std::string& delivery_token = std::string(), int previous_version_code = -1,
             std::vector<std::string> const& patch_formats = std::vector<std::string>(),
             const std::string& cert_hash = std::string(),
             const std::string& self_update_md5_cert_hash = std::string());



    void set_device_config_token(std::string value) {
        std::lock_guard<std::mutex> l (info_mutex);
        device_config_token = std::move(value);
    }

    void set_toc_cookie(std::string value) {
        std::lock_guard<std::mutex> l (info_mutex);
        toc_cookie = std::move(value);
    }

    struct bulk_details_request {
        std::string name;
        int installed_version_code = -1;
        bool include_details = false;

        bulk_details_request(std::string const& name) : name(name) {}
    };

    request_task bulk_details(std::vector<bulk_details_request> const& v);


};

}
