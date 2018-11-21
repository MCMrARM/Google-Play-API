#include <cstring>
#include <stdexcept>
#include <playapi/util/base64.h>

using namespace playapi;

char base64::table[65] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
unsigned char base64::reverse_table[256];
bool base64::reverse_table_initialized = false;

void base64::init_reverse_table() {
    memset(reverse_table, 255, sizeof(reverse_table));
    for (int i = 0; i < 64; i++) {
        reverse_table[table[i]] = (unsigned char) i;
    }
    reverse_table['='] = 64;
    reverse_table_initialized = true;
}

std::string base64::encode(const std::string& input) {
    size_t block_count = (input.size() + 2) / 3;
    std::string outp;
    outp.resize(block_count * 4);
    unsigned char i0, i1, i2, n0, n1, n2, n3;
    for (size_t i = 0; i < block_count; i++) {
        i0 = (unsigned char) input[i * 3];
        i1 = (unsigned char) (input.length() > i * 3 + 1 ? input[i * 3 + 1] : 0);
        i2 = (unsigned char) (input.length() > i * 3 + 2 ? input[i * 3 + 2] : 0);
        n0 = (unsigned char) ((i0 >> 2) & 0x3f);
        n1 = (unsigned char) (((i0 & 3) << 4) | ((i1 >> 4) & 0xf));
        n2 = (unsigned char) (((i1 & 0xf) << 2) | ((i2 >> 6) & 3));
        n3 = (unsigned char) (i2 & 0x3f);
        outp[i * 4] = table[n0];
        outp[i * 4 + 1] = table[n1];
        outp[i * 4 + 2] = input.length() > i * 3 + 1 ? table[n2] : '=';
        outp[i * 4 + 3] = input.length() > i * 3 + 2 ? table[n3] : '=';
    }
    return std::move(outp);
}

std::string base64::decode(const std::string& input, const char* skip_chars) {
    if (!reverse_table_initialized)
        init_reverse_table();
    std::string output;
    output.reserve((input.size() + 3) / 4 * 3);
    unsigned char i[4];
    size_t p = 0;
    while (p < input.size()) {
        for (int n = 0; n < 4; n++) {
            while (true) {
                if ((i[n] = reverse_table[input[p]]) != 255) {
                    if ((i[n] == 64 && n < 2) || (n == 3 && i[n - 1] == 64 && i[n] != 64))
                        throw std::runtime_error("Invalid '=' character");
                    p++;
                    break;
                }
                if (strchr(skip_chars, input[p]) == NULL)
                    throw std::runtime_error("Invalid character at " + std::to_string(p));
                if (++p == input.size())
                    throw std::runtime_error("Unexpected end of input");
            }
        }
        output.push_back((unsigned char) ((i[0] << 2) | ((i[1] >> 4) & 3)));
        if (i[2] != 64)
            output.push_back((unsigned char) (((i[1] & 0xf) << 4) | ((i[2] >> 2) & 0xf)));
        if (i[2] != 64 && i[3] != 64)
            output.push_back((unsigned char) (((i[2] & 3) << 6) | i[3]));
    }
    return output;
}
