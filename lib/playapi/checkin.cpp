#include <playapi/checkin.h>

#include <ctime>
#include <sstream>
#include <gsf.pb.h>
#include <playapi/device_info.h>
#include <playapi/login.h>
#include <playapi/util/rand.h>
#include <playapi/util/http.h>
#include <playapi/http_task.h>

using namespace playapi;

std::string checkin_result::get_string_android_id() const {
    std::stringstream ss;
    ss << std::hex << android_id;
    return ss.str();
}

checkin_api::checkin_api(device_info const& device) : device(device) {
    //
}

task_ptr<void> checkin_api::add_auth(login_api& login) {
    return login.fetch_service_auth_cookie("ac2dm", "com.google.android.gsf", login_api::certificate::google, true)->
            then<void>([this, &login](std::string&& token) {
        auth_user user;
        user.email = login.get_email();
        user.auth_cookie = token;
        assert(user.email.length() > 0 && user.auth_cookie.length() > 0);
        auth.push_back(user);
    });
}

task_ptr<checkin_result> checkin_api::perform_checkin(const checkin_result& last) {
    assert(auth.size() > 0);

    // build checkin request
    proto::gsf::AndroidCheckinRequest req;
    req.set_version(3);
    req.set_fragment(0);
    proto::gsf::AndroidCheckinProto* checkin = req.mutable_checkin();
    checkin->set_devicetype((int) device.type);
    if (last.time == 0) {
        checkin->set_lastcheckinmsec(0);
        proto::gsf::AndroidEventProto* event = checkin->add_event();
        event->set_tag("event_log_start");
        event->set_timemsec(std::time(nullptr) * 1000LL -
                            rand::next_int<long long>(30LL * 1000LL, 5L * 60LL * 1000LL)); // 30s-5m before now
    } else {
        checkin->set_lastcheckinmsec(last.time);
        req.set_securitytoken(last.security_token);
        req.set_devicedataversioninfo(last.device_data_version_info);
    }
    if (device.gservices_digest.length() > 0)
        req.set_digest(device.gservices_digest);
    if (last.android_id != 0)
        req.set_id((long long) last.android_id);

    proto::gsf::AndroidBuildProto* build = checkin->mutable_build();
    if (device.build_fingerprint.length() > 0)
        build->set_id(device.build_fingerprint);
    if (device.build_product.length() > 0)
        build->set_product(device.build_product);
    if (device.build_brand.length() > 0)
        build->set_carrier(device.build_brand);
    if (device.build_radio.length() > 0)
        build->set_radio(device.build_radio);
    if (device.build_bootloader.length() > 0)
        build->set_bootloader(device.build_bootloader);
    if (device.build_device.length() > 0)
        build->set_device(device.build_device);
    if (device.build_model.length() > 0)
        build->set_model(device.build_model);
    if (device.build_manufacturer.length() > 0)
        build->set_manufacturer(device.build_manufacturer);
    if (device.build_build_product.length() > 0)
        build->set_buildproduct(device.build_build_product);
    build->set_timestamp(device.build_timestamp);
    build->set_sdkversion(device.build_sdk_version);
    build->set_otainstalled(device.build_ota_installed);

    if (device.build_client.length() > 0)
        build->set_client(device.build_client);
    build->set_googleservices(device.build_google_services);
    for (const auto& e : device.build_google_packages) {
        proto::gsf::AndroidBuildProto::PackageVersion* pkg = build->add_googlepackage();
        pkg->set_name(e.first);
        pkg->set_version(e.second);
    }
    if (device.build_security_patch.length() > 0)
        build->set_securitypatch(device.build_security_patch);

    device.fill_device_config_proto(*req.mutable_deviceconfiguration(), true);
    checkin->set_voicecapable(device.voice_capable);

    req.set_locale(device.locale);
    req.set_timezone(device.time_zone);
    req.set_loggingid(device.random_logging_id);

    if (device.roaming.length() > 0)
        checkin->set_roaming(device.roaming);
    if (device.cell_operator.length() > 0)
        checkin->set_celloperator(device.cell_operator);
    if (device.sim_operator.length() > 0)
        checkin->set_celloperator(device.sim_operator);
    checkin->set_usernumber(device.user_number);
    req.set_userserialnumber(device.user_serial_number);

    if (device.mac_addr_type.size() > 0 && device.mac_addr.size() > 0) {
        req.add_macaddrtype(device.mac_addr_type);
        req.add_macaddr(device.mac_addr);
    }
    if (device.meid.size() > 0)
        req.set_meid(device.meid);
    if (device.serial_number.size() > 0)
        req.set_serialnumber(device.serial_number);
    for (const auto& e : device.ota_certs)
        req.add_otacert(e);
    auth_mutex.lock();
    for (const auto& user : auth) {
        req.add_accountcookie("[" + user.email + "]");
        req.add_accountcookie(user.auth_cookie);
    }
    auth_mutex.unlock();

    http_request http("https://android.clients.google.com/checkin");
    http.add_header("Content-type", "application/x-protobuffer");
    http.set_encoding("gzip,deflate");
    http.add_header("Content-encoding", "gzip");
    http.add_header("Accept-encoding", "gzip");
    http.set_method(http_method::POST);
    http.set_user_agent(
            "Dalvik/2.1.0 (Linux; U; Android " + device.build_version_string + "; " + device.build_model + " Build/" +
            device.build_id + ")");
#ifndef NDEBUG
    printf("Checkin data: %s\n", req.DebugString().c_str());
#endif
    http.set_gzip_body(req.SerializeAsString());

    return http_task::make(http)->then<checkin_result>([](http_response&& http_resp) {
        if (!http_resp)
            throw std::runtime_error("Failed to send checkin");

        proto::gsf::AndroidCheckinResponse resp;
        if (!resp.ParseFromString(http_resp.get_body()))
            throw std::runtime_error("Failed to parse checkin");
#ifndef NDEBUG
        printf("Checkin response: %s\n", resp.DebugString().c_str());
#endif
        checkin_result res;
        res.time = resp.timemsec();
        res.android_id = resp.androidid();
        res.security_token = resp.securitytoken();
        res.device_data_version_info = resp.devicedataversioninfo();
        return res;
    });
}