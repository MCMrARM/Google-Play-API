#include <playapi/file_login_cache.h>
#include <fstream>
#include <playapi/util/config.h>

using namespace playapi;

void file_login_cache::load() {
    std::lock_guard<std::mutex> l (mutex);
    config c;
    {
        std::ifstream fs(path);
        c.load(fs);
    }
    int n = c.get_int("token_count", 0);
    for (int i = 0; i < n; i++) {
        auto p = "token." + std::to_string(i) + ".";
        auto exp = std::chrono::system_clock::time_point(std::chrono::milliseconds(c.get_long(p + "expires")));
        auth_cookies[{c.get(p + "service"), c.get(p + "app")}] = {c.get(p + "token"), exp};
    }
}

void file_login_cache::save() {
    std::lock_guard<std::mutex> l (mutex);
    config c;
    int count = 0;
    for (auto const& e : auth_cookies) {
        if (e.second.second <= std::chrono::system_clock::now())
            continue;
        auto p = "token." + std::to_string(count) + ".";
        c.set(p + "service", e.first.first);
        c.set(p + "app", e.first.second);
        c.set(p + "token", e.second.first);
        c.set_long(p + "expires", std::chrono::duration_cast<std::chrono::milliseconds>(
                e.second.second.time_since_epoch()).count());
        count++;
    }
    c.set_int("token_count", count);
    {
        std::ofstream fs(path);
        c.save(fs);
    }
}