#pragma once

#include <string>
#include <vector>
#include <playapi/util/http.h>
#include <play_respone.pb.h>
#include <play_settings.pb.h>
#include <play_contentsync.pb.h>
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
        bool is_protobuf_content = false;
    };

private:

    device_info& device;
    std::string url;

    std::string auth_email, auth_token;
    checkin_result checkin_data;

    std::string build_user_agent();

    void add_headers(http_request& req, const request_options& options);

public:

    std::string device_config_token;
    std::string toc_cookie;
    experiments_list experiments;

    api(device_info& device, const std::string& url = "https://android.clients.google.com/fdfe/") : device(device),
                                                                                                    url(url) {}

    void set_auth(login_api& login);

    void set_checkin_data(const checkin_result& result);

    proto::finsky::response::ResponseWrapper
    send_request(http_method method, const std::string& path, const request_options& options);

    proto::finsky::response::ResponseWrapper
    send_request(http_method method, const std::string& path, const std::string& bin_data,
                 const request_options& options);

    proto::finsky::response::ResponseWrapper
    send_request(http_method method, const std::string& path,
                 const std::vector<std::pair<std::string, std::string>>& pairs, const request_options& options);

    proto::finsky::response::ResponseWrapper fetch_user_settings();

    proto::finsky::response::ResponseWrapper fetch_toc();

    proto::finsky::response::ResponseWrapper
    upload_device_config(const std::string& gcm_reg_id = "", bool upload_full_config = true);

    proto::finsky::response::ResponseWrapper accept_tos(const std::string& token, bool allow_marketing_emails = false);

    proto::finsky::response::ResponseWrapper
    get_search_suggestions(std::string q, int backend_id = 3, int icon_size = 120, bool request_navigational = true);

    proto::finsky::response::ResponseWrapper search(const std::string& q, int backend_id = 3);

    proto::finsky::response::ResponseWrapper details(const std::string& app);

    proto::finsky::response::ResponseWrapper
    delivery(const std::string& app, int version_code, const std::string& library_token,
             const std::string& delivery_token = std::string(), int previous_version_code = -1,
             std::vector<std::string> const& patch_formats = std::vector<std::string>(),
             const std::string& cert_hash = std::string(),
             const std::string& self_update_md5_cert_hash = std::string());

    proto::finsky::response::ResponseWrapper content_sync(
            proto::finsky::contentsync::ContentSyncRequestProto const& req);

    proto::finsky::response::ResponseWrapper ack_notification(std::string const& nid);

    struct bulk_details_request {
        std::string name;
        int installed_version_code = -1;
        bool include_details = false;
    };

    proto::finsky::response::ResponseWrapper bulk_details(std::vector<bulk_details_request> const& v);


};

}