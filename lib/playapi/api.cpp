#include <playapi/api.h>

#include <cassert>
#include <algorithm>
#include <play_respone.pb.h>
#include <play_device_config.pb.h>
#include <playapi/device_info.h>
#include <playapi/login.h>
#include <playapi/util/base64.h>

using namespace playapi;

task_ptr<void> api::set_auth(login_api& login) {
    std::lock_guard<std::mutex> l(auth_mutex);
    this->login = &login;
    return login.fetch_service_auth_cookie("androidmarket", "com.android.vending", login_api::certificate::google)->
            then<void>([this](std::string&& token) {
        std::lock_guard<std::mutex> l(auth_mutex);
        auth_email = this->login->get_email();
        auth_token = token;
    });
}

task_ptr<void> api::invalidate_token() {
    return login->fetch_service_auth_cookie("androidmarket", "com.android.vending", login_api::certificate::google, true)->
            then<void>([this](std::string&& token) {
        std::lock_guard<std::mutex> l(auth_mutex);
        auth_email = this->login->get_email();
        auth_token = token;
    });
}

void api::set_checkin_data(const checkin_result& result) {
    std::lock_guard<std::mutex> l(auth_mutex);
    checkin_data = result;
}

std::string api::build_user_agent() {
    std::stringstream ua;
    ua << "Android-Finsky/14.2.63-all [0] [PR] 240807813 (api=3,versionCode=81426300,";
    ua << "sdk=" << device.build_sdk_version << ",";
    ua << "device=" << device.build_device << ",";
    ua << "hardware=" << device.build_product << ",";
    ua << "product=" << device.build_build_product << ",";
    ua << "platformVersionRelease=" << device.build_version_string << ",";
    ua << "buildId=" << device.build_id << ",";
    ua << "isWideScreen=" << (device.wide_screen ? "1" : "0") << ",";
    ua << "supportedAbis=";
    bool first = true;
    for (auto const& abi : device.config_native_platforms) {
        if (!first)
            ua << ";";
        ua << abi;
        first = false;
    }
    ua << ")";
    return ua.str();
}

void api::add_headers(http_request& req, const request_options& options) {
    std::lock_guard<std::mutex> l(auth_mutex);
    std::lock_guard<std::mutex> l2 (info_mutex);
    assert(auth_token.length() > 0);
    req.set_user_agent(build_user_agent());
    req.add_header("Authorization", "GoogleLogin auth=" + auth_token);
    if (options.content_type == request_content_type::protobuf)
        req.add_header("Content-Type", "application/x-protobuf");
    std::string locale = device.locale;
    std::replace(locale.begin(), locale.end(), '_', '-');
    req.add_header("Accept-Language", locale);
    if (checkin_data.android_id != 0)
        req.add_header("X-DFE-Device-Id", checkin_data.get_string_android_id());
    if (options.include_content_filters)
        req.add_header("X-DFE-Content-Filters", "");
    if (options.include_network_type)
        req.add_header("X-DFE-Network-Type", "4");
    req.add_header("X-DFE-Request-Params", "timeoutMs=2500");
    req.add_header("X-DFE-Client-Id", "am-android-google");
    if (options.include_checkin_consistency_token && !checkin_data.device_data_version_info.empty())
        req.add_header("X-DFE-Device-Checkin-Consistency-Token", checkin_data.device_data_version_info);
    if (options.include_device_config_token && !device_config_token.empty())
        req.add_header("X-DFE-Device-Config-Token", device_config_token);
    if (options.include_toc_cookie && !toc_cookie.empty())
        req.add_header("X-DFE-Cookie", toc_cookie);
    experiments.add_headers(req);
}

api::request_task api::send_request(http_method method, const std::string& path, const std::string& bin_data,
                                    const request_options& options) {
    using ret_type = proto::finsky::response::ResponseWrapper;
    http_request req(url + path);
    req.set_method(method);
    add_headers(req, options);
    req.set_body(bin_data);
    return http_task::make(req)->then<ret_type>([this, method, path, bin_data, options](http_response&& resp) {
        if (!resp)
            throw std::runtime_error("Failed to send request");
        if (resp.get_status_code() == 401) {
            return invalidate_token()->then<ret_type>([this, method, path, bin_data, options]() {
                return send_request(method, path, bin_data, options);
            });
        }
        ret_type ret;
        if (!ret.ParseFromString(resp.get_body()))
            throw std::runtime_error("Failed to parse response");
#ifndef NDEBUG
        printf("api response body = %s\n", ret.DebugString().c_str());
#endif
        if (ret.has_targets()) {
            std::lock_guard<std::mutex> lock (info_mutex);
            experiments.set_targets(ret.targets());
        }
        return pre_set_task<ret_type>::make(ret);
    });
}

api::request_task api::send_request(http_method method, const std::string& path, const request_options& options) {
    return send_request(method, path, std::string(), options);
}

api::request_task api::send_request(http_method method, const std::string& path,
                                    const std::vector<std::pair<std::string, std::string>>& pairs,
                                    const request_options& options) {
    url_encoded_entity u;
    for (const auto& pair : pairs)
        u.add_pair(pair.first, pair.second);
    return send_request(method, path, u.encode(), options);
}

api::request_task api::fetch_user_settings() {
    return send_request(http_method::GET, "userSettings", request_options());
}

api::request_task api::fetch_toc() {
    request_options opt;
    opt.include_device_config_token = true;
    opt.include_toc_cookie = true;
    return send_request(http_method::GET, "toc", opt);
}

api::request_task api::upload_device_config(const std::string& gcm_reg_id, bool upload_full_config) {
    request_options opt;
    opt.content_type = request_content_type::protobuf;
    opt.include_device_config_token = true;

    proto::finsky::device_config::UploadDeviceConfigRequest req;
    if (upload_full_config)
        device.fill_device_config_proto(*req.mutable_deviceconfiguration());
    if (gcm_reg_id.length() > 0)
        req.set_gcmregistrationid(gcm_reg_id);
    req.mutable_shortdescription()->set_brand(device.build_brand);
    req.mutable_shortdescription()->set_manufacturer(device.build_manufacturer);
    req.mutable_shortdescription()->set_fingerprint(device.build_fingerprint);
    req.mutable_shortdescription()->set_usercount(1);
    req.mutable_shortdescription()->set_securitypatch(device.build_security_patch);
    req.mutable_dataservicesubscriber();
#ifndef NDEBUG
    printf("Upload Device Config: %s\n", req.DebugString().c_str());
#endif
    return send_request(http_method::POST, "uploadDeviceConfig", req.SerializeAsString(), opt);
}

api::request_task api::accept_tos(const std::string& token, bool allow_marketing_emails) {
    url_encoded_entity e;
    e.add_pair("toscme", allow_marketing_emails ? "true" : "false");
    e.add_pair("tost", token);
    return send_request(http_method::POST, "acceptTos", e.encode(), request_options());
}

api::request_task api::get_search_suggestions(std::string q, int backend_id, int icon_size, bool request_navigational) {
    url_encoded_entity e;
    e.add_pair("q", q);
    e.add_pair("c", std::to_string(backend_id));
    e.add_pair("ssis", std::to_string(icon_size));
    e.add_pair("sst", "2");
    if (request_navigational)
        e.add_pair("sst", "3");
    return send_request(http_method::GET, "searchSuggest?" + e.encode(), request_options());
}

api::request_task api::search(const std::string& q, int backend_id) {
    url_encoded_entity e;
    e.add_pair("c", std::to_string(backend_id));
    e.add_pair("q", q);
    return send_request(http_method::GET, "search?" + e.encode(), request_options());
}

api::request_task api::details(const std::string& app) {
    url_encoded_entity e;
    e.add_pair("doc", app);
    return send_request(http_method::GET, "details?" + e.encode(), request_options());
}

api::request_task api::delivery(const std::string& app, int version_code, const std::string& library_token,
                                const std::string& delivery_token, int previous_version_code,
                                const std::vector<std::string>& patch_formats, const std::string& cert_hash,
                                const std::string& self_update_md5_cert_hash) {
    url_encoded_entity e;
    e.add_pair("doc", app);
    e.add_pair("ot", "1");
    if (library_token.length() > 0)
        e.add_pair("st", library_token);
    if (version_code != -1)
        e.add_pair("vc", std::to_string(version_code));
    if (previous_version_code != -1) {
        e.add_pair("bvc", std::to_string(previous_version_code));
        for (auto& p : patch_formats)
            e.add_pair("pf", p);
    }
    if (self_update_md5_cert_hash.length() > 0)
        e.add_pair("shh", self_update_md5_cert_hash);
    if (cert_hash.length() > 0)
        e.add_pair("ch", cert_hash);
    e.add_pair("fdcf", "1");
    e.add_pair("fdcf", "2");
    if (delivery_token.length() > 0)
        e.add_pair("dtok", delivery_token);
    return send_request(http_method::GET, "delivery?" + e.encode(), request_options());
}

api::request_task api::bulk_details(std::vector<bulk_details_request> const& v) {
    request_options opt;
    opt.include_device_config_token = true;
    opt.content_type = request_content_type::protobuf;

    proto::finsky::details::BulkDetailsRequest req;
    for (auto const& p : v) {
        auto e = req.add_entry();
        e->set_docid(p.name);
        if (p.installed_version_code != -1)
            e->set_installedversioncode(p.installed_version_code);
        e->set_includedetails(p.include_details);
    }
    return send_request(http_method::POST, "bulkDetails", req.SerializeAsString(), opt);
}