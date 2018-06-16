#include "config.h"

#include <fstream>
#include "../include/playapi/device_info.h"
#include "../include/playapi/api.h"

using namespace playapi;

void app_config::load() {
    std::ifstream s(path);
    config.load(s);

    user_email = config.get("user_email");
    user_token = config.get("user_token");
}

void app_config::save() {
    config.set("user_email", user_email);
    config.set("user_token", user_token);

    std::ofstream s(path);
    config.save(s);
}

void device_config::load() {
    std::ifstream s(path);
    config.load(s);

    checkin_data.time = config.get_long("checkin.time", checkin_data.time);
    checkin_data.android_id = (unsigned long long) config.get_long("checkin.android_id", checkin_data.android_id);
    checkin_data.security_token = (unsigned long long) config.get_long("checkin.security_token", checkin_data.security_token);
    checkin_data.device_data_version_info = config.get("checkin.device_data_version_info", checkin_data.device_data_version_info);
}

void device_config::save() {
    config.set_long("checkin.time", checkin_data.time);
    config.set_long("checkin.android_id", checkin_data.android_id);
    config.set_long("checkin.security_token", checkin_data.security_token);
    config.set("checkin.device_data_version_info", checkin_data.device_data_version_info);

    std::ofstream s(path);
    config.save(s);
}

void device_config::load_device_info_data(playapi::device_info& dev) {
    dev.generated_mac_addr = config.get("generated_mac_addr", dev.generated_mac_addr);
    dev.generated_meid = config.get("generated_meid", dev.generated_meid);
    dev.generated_serial_number = config.get("generated_serial_number", dev.generated_serial_number);
    dev.random_logging_id = config.get_long("random_logging_id", dev.random_logging_id);
}

void device_config::set_device_info_data(const playapi::device_info& dev) {
    config.set("generated_mac_addr", dev.generated_mac_addr);
    config.set("generated_meid", dev.generated_meid);
    config.set("generated_serial_number", dev.generated_serial_number);
    config.set_long("random_logging_id", dev.random_logging_id);
}

void device_config::load_api_data(const std::string& email, playapi::api& api) {
    std::string p = "api." + email + ".";
    api.device_config_token = config.get(p + "device_config_token", api.device_config_token);
    api.toc_cookie = config.get(p + "toc_cookie", api.toc_cookie);
    api.experiments.set_targets(config.get(p + "experiments"));
}

void device_config::set_api_data(const std::string& email, const playapi::api& api) {
    std::string p = "api." + email + ".";
    config.set(p + "device_config_token", api.device_config_token);
    config.set(p + "toc_cookie", api.toc_cookie);
    config.set(p + "experiments", api.experiments.get_comma_separated_target_list());
}