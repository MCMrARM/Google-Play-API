#pragma once

#include <string>
#include <map>
#include <vector>
#include <regex>

namespace playapi {

class config {

private:

    struct entry {
        bool is_array;
        std::string text;
        std::vector<std::string> array;
    };

    std::map<std::string, entry> entries;

    static std::regex escape_key_regex;
    static std::regex unescape_key_regex;
    static std::regex escape_val_regex;
    static std::regex unescape_val_regex;
    static std::regex unescape_regex;

public:

    static std::istream& read_line(std::istream& stream, std::string& line);

    static void write_value(const std::string& value, std::ostream& stream, bool array = false);

    static std::string unescape_value(const std::string& value);

    /* formatting options */
    size_t indent = 4;
    bool array_append_comma = false;

    void set(std::string name, std::string val);

    void set_int(std::string name, int val);

    void set_long(std::string name, long long val);

    void set_bool(std::string name, bool b);

    void set_array(std::string name, std::vector<std::string> val);


    std::string get(std::string name, std::string default_val = "") const;

    int get_int(std::string name, int default_val = 0) const;

    long long get_long(std::string name, long long default_val = 0) const;

    bool get_bool(std::string name, bool default_val = false) const;

    std::vector<std::string>
    get_array(std::string name, std::vector<std::string> default_val = std::vector<std::string>()) const;


    void load(std::istream& stream);

    void save(std::ostream& stream);

};

}