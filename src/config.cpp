#include "config.h"

#include <fstream>
#include <playdl/config.h>
#include <playdl/device_info.h>

using namespace playdl;

void app_config::load() {
    config c;
    std::ifstream s("playdl.conf");
    c.load(s);

    user_email = c.get("user_email");
    user_token = c.get("user_token");
}

void app_config::save() {
    config c;

    c.set("user_email", user_email);
    c.set("user_token", user_token);

    std::ofstream s("playdl.conf");
    c.save(s);
}

void app_config::load_device(const std::string& device_path, playdl::device_info& device) {
    config c;
    std::ifstream s(device_path + ".state");
    c.load(s);
    device.last_checkin_time = c.get_long("last_checkin_time", device.last_checkin_time);
    device.android_id = (unsigned long long) c.get_long("android_id", device.android_id);
    device.security_token = (unsigned long long) c.get_long("security_token", device.security_token);
    device.device_data_version_info = c.get("device_data_version_info", device.device_data_version_info);
    device.random_logging_id = c.get_long("random_logging_id", device.random_logging_id);
    device.generated_mac_addr = c.get("generated_mac_addr", device.generated_mac_addr);
    device.generated_meid = c.get("generated_meid", device.generated_meid);
    device.generated_serial_number = c.get("generated_serial_number", device.generated_serial_number);
}

void app_config::save_device(const std::string& device_path, playdl::device_info& device) {
    config c;

    if (device.last_checkin_time != 0)
        c.set_long("last_checkin_time", device.last_checkin_time);
    if (device.android_id != 0)
        c.set_long("android_id", device.android_id);
    if (device.security_token != 0)
        c.set_long("security_token", device.security_token);
    if (device.device_data_version_info.length() > 0)
        c.set("device_data_version_info", device.device_data_version_info);
    if (device.random_logging_id != 0)
        c.set_long("random_logging_id", device.random_logging_id);
    if (device.generated_mac_addr.length() > 0)
        c.set("generated_mac_addr", device.generated_mac_addr);
    if (device.generated_meid.length() > 0)
        c.set("generated_meid", device.generated_meid);
    if (device.generated_serial_number.length() > 0)
        c.set("generated_serial_number", device.generated_serial_number);

    std::ofstream s(device_path + ".state");
    c.save(s);
}