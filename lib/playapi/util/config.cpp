#include <iostream>
#include <playapi/util/config.h>

#include <regex>

using namespace playapi;

std::regex config::escape_key_regex("=|\\\\|;");
std::regex config::unescape_key_regex("\\\\(?==|;)"); // remove backslashes before special chars
std::regex config::escape_val_regex("\"|\n|\\\\");
std::regex config::unescape_val_regex("\\\\(?=\"|\n)"); // remove backslashes before special chars
std::regex config::unescape_regex("\\\\\\\\"); // change double backslashes to a single backslash

std::string config::get(std::string name, std::string default_val) const {
    if (entries.count(name) > 0 && !entries.at(name).is_array)
        return entries.at(name).text;
    return default_val;
}

int config::get_int(std::string name, int default_val) const {
    if (entries.count(name) > 0 && !entries.at(name).is_array)
        return std::stoi(entries.at(name).text);
    return default_val;
}

long long config::get_long(std::string name, long long default_val) const {
    if (entries.count(name) > 0 && !entries.at(name).is_array)
        return std::stoll(entries.at(name).text);
    return default_val;
}

bool config::get_bool(std::string name, bool default_val) const {
    if (entries.count(name) > 0 && !entries.at(name).is_array) {
        const std::string& str = entries.at(name).text;
        return (str == "true" || str == "1" || str == "on");
    }
    return default_val;
}

std::vector<std::string> config::get_array(std::string name, std::vector<std::string> default_val) const {
    if (entries.count(name) > 0 && entries.at(name).is_array)
        return entries.at(name).array;
    return default_val;
}

void config::set(std::string name, std::string val) {
    entries[name] = {false, val, {}};
}

void config::set_int(std::string name, int val) {
    entries[name] = {false, std::to_string(val), {}};
}

void config::set_long(std::string name, long long val) {
    entries[name] = {false, std::to_string(val), {}};
}

void config::set_bool(std::string name, bool b) {
    entries[name] = {false, b ? "true" : "false", {}};
}

void config::set_array(std::string name, std::vector<std::string> val) {
    entries[name] = {true, std::string(), val};
}

std::string config::unescape_value(const std::string& value) {
    if (value.length() > 0 && value[0] == '"') {
        if (value[value.size() - 1] != '"')
            throw std::runtime_error("Badly formatted value: starts with a quote, but doesn't end with one");
        std::string ret = value.substr(1, value.size() - 2);
        ret = std::regex_replace(ret, unescape_val_regex, "");
        ret = std::regex_replace(ret, unescape_regex, "\\");
        return ret;
    } else {
        return value;
    }
}

void config::write_value(const std::string& value, std::ostream& stream, bool array) {
    if (value.length() > 0 && (value[0] == '[' || value[0] == '"' || value[0] == ' ' ||
                               value[value.length() - 1] == '\\' || value.find('\n') != std::string::npos ||
                               (array && value[value.length() - 1] == ','))) {
        // must quote & escape
        stream << '"' << std::regex_replace(value, escape_val_regex, "\\$&") << '"';
    } else {
        stream << value;
    }
}

std::istream& config::read_line(std::istream& stream, std::string& line) {
    while (std::getline(stream, line)) {
        if (line.size() == 0 || line[0] == ';')
            continue;
        while (line[line.length() - 1] == '\\') {
            line += '\n';
            std::string tmp_line;
            if (!std::getline(stream, tmp_line))
                throw std::runtime_error("Bad formatted config line: backslash at the end of the last line.");
            line += tmp_line;
        }
        return stream;
    }
    return stream;
}

void config::load(std::istream& stream) {
    std::string line;
    while (read_line(stream, line)) {
        size_t pos = line.find(" = ");
        if (pos == std::string::npos)
            continue;
        std::string key = line.substr(0, pos);
        key = std::regex_replace(key, unescape_key_regex, "");
        key = std::regex_replace(key, unescape_regex, "\\");
        std::string val = line.substr(pos + 3);
        if (val == "[") {
            // array
            std::vector<std::string> vals;
            while (true) {
                if (!read_line(stream, line))
                    throw std::runtime_error("An array was opened, but was not closed.");
                if (line == "]")
                    break;
                size_t line_indent;
                for (line_indent = 0; line_indent < line.size(); line_indent++) {
                    if (line[line_indent] != ' ' && line[line_indent] != '\t')
                        break;
                }
                if (line[line.length() - 1] == ',')
                    line = line.substr(0, line.length() - 1);
                vals.push_back(unescape_value(line.substr(line_indent)));
            }
            set_array(key, vals);
            continue;
        }
        set(key, unescape_value(val));
    }
}

void config::save(std::ostream& stream) {
    for (const auto& e : entries) {
        stream << std::regex_replace(e.first, escape_key_regex, "\\$&") << " = ";
        if (e.second.is_array) {
            stream << "[";
            bool f = true;
            for (const auto& a : e.second.array) {
                if (!f) {
                    if (array_append_comma)
                        stream << ",\n";
                } else
                    f = false;
                stream << '\n';
                for (int i = 0; i < indent; i++)
                    stream << ' ';
                write_value(a, stream, true);
            }
            stream << "\n]\n";
        } else {
            write_value(e.second.text, stream);
            stream << '\n';
        }
    }
}
