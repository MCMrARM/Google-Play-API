#include "httputils.h"

#include <cassert>
#include <sstream>
#include <zlib.h>

using namespace playapi;

void url_encoded_entity::add_pair(const std::string& key, const std::string& val) {
    pairs[key] = val;
}

std::string url_encoded_entity::encode(CURL* curl) const {
    std::stringstream ss;
    bool f = true;
    for (const auto& pair : pairs) {
        if (f)
            f = false;
        else
            ss << "&";

        char* escapedKey = curl_easy_escape(curl, pair.first.c_str(), (int) pair.first.length());
        char* escapedVal = curl_easy_escape(curl, pair.second.c_str(), (int) pair.second.length());
        ss << escapedKey << "=" << escapedVal;
        curl_free(escapedKey);
        curl_free(escapedVal);
    }
    return ss.str();
}

std::string url_encoded_entity::encode() const {
    CURL* curl = curl_easy_init();
    assert(curl != nullptr);
    std::string ret = encode(curl);
    curl_easy_cleanup(curl);
    return std::move(ret);
}

http_response::http_response(CURL* curl, CURLcode curlCode, std::string body) : curl(curl), curlCode(curlCode),
                                                                                body(body) {
    //
}

http_response::http_response(http_response&& r) : curl(r.curl), curlCode(r.curlCode), body(r.body) {
    r.curl = nullptr;
    r.curlCode = CURLE_FAILED_INIT;
    r.body = std::string();
}

http_response& http_response::operator=(http_response&& r) {
    curl = r.curl;
    curlCode = r.curlCode;
    body = r.body;
    r.curl = nullptr;
    r.curlCode = CURLE_FAILED_INIT;
    r.body = std::string();
    return *this;
}

http_response::~http_response() {
    curl_easy_cleanup(curl);
    curl = nullptr;
}

void http_request::set_body(const url_encoded_entity& ent) {
    set_body(ent.encode());
}

void http_request::add_header(const std::string& key, const std::string& value) {
    headers[key] = value;
}

void http_request::set_gzip_body(const std::string& str) {
    z_stream zs;
    zs.zalloc = Z_NULL;
    zs.zfree = Z_NULL;
    zs.opaque = Z_NULL;
    int ret = deflateInit2(&zs, Z_BEST_COMPRESSION, Z_DEFLATED, 31, 8, Z_DEFAULT_STRATEGY);
    assert(ret == Z_OK);

    zs.avail_in = (uInt) str.length();
    zs.next_in = (unsigned char*) str.data();
    std::string out;
    while(true) {
        out.resize(out.size() + 4096);
        zs.avail_out = 4096;
        zs.next_out = (unsigned char*) out.data();
        ret = deflate(&zs, Z_FINISH);
        assert(ret != Z_STREAM_ERROR);
        if (zs.avail_out != 0) {
            out.resize(out.size() - zs.avail_out);
            break;
        }
    }
    deflateEnd(&zs);
    body = std::move(out);
}

size_t curl_stringstream_write_func(void* ptr, size_t size, size_t nmemb, std::stringstream* s) {
    s->write((char*) ptr, size * nmemb);
    return size * nmemb;
}

http_response http_request::perform() {
    CURL* curl = curl_easy_init();
    assert(curl != nullptr);
    assert(url.length() > 0);
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    switch (method) {
        case http_method::GET:
            curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
            break;
        case http_method::POST:
            curl_easy_setopt(curl, CURLOPT_POST, 1L);
            break;
        case http_method::PUT:
            curl_easy_setopt(curl, CURLOPT_PUT, 1L);
            break;
    }
    if (user_agent.length() > 0)
        curl_easy_setopt(curl, CURLOPT_USERAGENT, user_agent.c_str());
    if (encoding.length() > 0)
        curl_easy_setopt(curl, CURLOPT_ENCODING, encoding.c_str());
    if (body.length() > 0) {
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long) body.length());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
    }
    if (headers.size() > 0) {
        struct curl_slist* chunk = NULL;
        if (method == http_method::POST)
            chunk = curl_slist_append(chunk, "Expect:");
        for (const auto& header : headers) {
            chunk = curl_slist_append(chunk, (header.first + ": " + header.second).c_str());
        }
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);
    }
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);

#ifndef NDEBUG
    printf("http request: %s, body = %s\n", url.c_str(), body.c_str());
#endif
    std::stringstream output;
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &output);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_stringstream_write_func);
    CURLcode ret = curl_easy_perform(curl);
#ifndef NDEBUG
    printf("http response body: %s", output.str().c_str());
#endif
    return http_response(curl, ret, output.str());
}