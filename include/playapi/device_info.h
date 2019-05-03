#pragma once

#include <string>
#include <vector>
#include <map>

namespace playapi {

namespace proto { namespace gsf { class DeviceConfigurationProto; }}

class config;

enum class device_type {
    unknown = 1, phone, tablet, tv, glass, wearable = 7
};
static device_type device_type_from_string(const std::string& str);
static std::string device_type_to_string(device_type type);

enum class device_touch_screen {
    undefinied = 0, notouch, stylus, finger
};
static device_touch_screen device_touch_screen_from_string(const std::string& str);
static std::string device_touch_screen_to_string(device_touch_screen type);

enum class device_keyboard {
    undefinied = 0, nokeys, qwerty, twelvekey
};
static device_keyboard device_keyboard_from_string(const std::string& str);
static std::string device_keyboard_to_string(device_keyboard type);

enum class device_navigation {
    undefinied = 0, nonav, dpad, trackball, wheel
};
static device_navigation device_navigation_from_string(const std::string& str);
static std::string device_navigation_to_string(device_navigation type);

enum class device_screen_layout {
    undefinied = 0, small, normal, large, xlarge
};
static device_screen_layout device_screen_layout_from_string(const std::string& str);
static std::string device_screen_layout_to_string(device_screen_layout type);

struct device_info {

    // constants

    // some made up data to fall back to in case user doesn't specify a valid device
    device_type type = device_type::tablet;
    std::string build_fingerprint = "diy/desktop/desktop:6.0/DSKTOP/desktop:user/release-keys";
    std::string build_id = "DSKTOP";
    std::string build_product = "desktop";
    std::string build_brand = "diy";
    std::string build_radio;
    std::string build_bootloader = "unknown";
    std::string build_client = "android-google";
    long long build_timestamp = 1480546800;
    int build_google_services = 10084470;
    std::string build_device = "D001";
    int build_sdk_version = 23;
    std::string build_version_string = "6.0.1";
    std::string build_model = "DIY_D001";
    std::string build_manufacturer = "diy";
    std::string build_build_product = "desktop";
    bool build_ota_installed = false;
    std::vector<std::pair<std::string, int>> build_google_packages;
    std::string build_security_patch = "2019-01-05";

    device_touch_screen config_touch_screen = device_touch_screen::finger;
    device_keyboard config_keyboard = device_keyboard::qwerty;
    device_navigation config_navigation = device_navigation::nonav;
    device_screen_layout config_screen_layout = device_screen_layout::xlarge;
    bool config_has_hard_keyboard = false;
    bool config_has_five_way_navig = false;
    int config_screen_density = 120;
    int config_gles_version = 0x30000;
    std::vector<std::string> config_system_shared_libraries = {"android.test.runner",
                                                               "com.android.future.usb.accessory",
                                                               "com.android.location.provider",
                                                               "com.google.android.gms", "javax.obex",
                                                               "org.apache.http.legacy"};
    std::vector<std::pair<std::string, int>> config_system_features; // this will be initialized in the constructor
    std::vector<std::string> config_native_platforms = {"x86", "armeabi-x7a", "armeabi"};
    int config_screen_width = 1920;
    int config_screen_height = 1080;
    std::vector<std::string> config_system_supported_locales = {"en", "en-US"};
    std::vector<std::string> config_gl_extensions;
    int config_smallest_screen_width_dp = 1080;
    bool config_low_ram = false;
    long long config_total_ram = 4093796352;
    int config_cores = 4;
    bool voice_capable = false; // this is not in config because it's somewhere else in the proto file
    bool wide_screen = false;
    std::vector<std::string> ota_certs;

    bool config_keyguard_device_secure = false;
    std::string country = "us";
    std::string locale = "en_US";
    std::string time_zone = "America/New_York";

    std::string roaming = "WIFI::";
    std::string cell_operator;
    std::string sim_operator;
    int user_number = 0;
    int user_serial_number = 0;

    std::string gservices_digest; // digest of the gservices config

    std::string mac_addr_type = "wifi";
    std::string mac_addr;
    bool mac_addr_generate = true;
    std::string meid;
    bool meid_generate = false;
    std::string serial_number;
    bool serial_number_generate = false;
    std::string serial_number_generate_chars;
    int serial_number_generate_length = 8;

    // fields

    long long random_logging_id = 0;

    std::string generated_mac_addr;
    std::string generated_meid;
    std::string generated_serial_number;

    device_info();

    void load(config& conf);

    void generate_fields();

    void fill_device_config_proto(proto::gsf::DeviceConfigurationProto& proto,
                                  bool feature_add_gles_version_if_zero = false) const;

};

}