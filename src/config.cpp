#include "config.h"

#include <fstream>
#include <playdl/config.h>

using namespace playdl;

void app_config::load() {
    config c;
    std::ifstream s ("playdl.conf");
    c.load(s);

    user_email = c.get("user_email");
    user_token = c.get("user_token");
}

void app_config::save() {
    config c;

    c.set("user_email", user_email);
    c.set("user_token", user_token);

    std::ofstream s ("playdl.conf");
    c.save(s);
}