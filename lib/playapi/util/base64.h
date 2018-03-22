#pragma once

#include <string>

namespace playapi {

class base64 {

public:
    class character_table {
    private:
        friend class base64;
        const char* table;
        unsigned char reverse_table[256];

    public:
        character_table(const char table[65]);
    };

    static character_table standard;
    static character_table url_safe;

    static std::string encode(const std::string& input, character_table& table = standard);

    static std::string decode(const std::string& input, character_table& table = standard,
                              bool allow_no_pad = false, const char* skip_chars = "\r\n");

};

}