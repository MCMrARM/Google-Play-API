#pragma once

#include <string>

namespace playapi {

class base64 {

private:

    static char table[65];
    static unsigned char reverse_table[256];
    static bool reverse_table_initialized;

    static void init_reverse_table();

public:

    static std::string encode(const std::string& input);

    static std::string decode(const std::string& input, const char* skip_chars = "\r\n");

};

}