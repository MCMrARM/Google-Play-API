#pragma once

#include <string>
#include <map>
#include <vector>
#include <functional>
#include <curl/curl.h>

namespace playapi {

class http_request;

class url_encoded_entity {

public:

    std::vector<std::pair<std::string, std::string>> pairs;

    void add_pair(const std::string& key, const std::string& val);

    std::string encode(CURL* curl) const;

    std::string encode() const;

};

struct http_response {

private:

    CURL* curl;
    CURLcode curlCode;
    std::string body;

public:

    http_response(CURL* curl, CURLcode curlCode, std::string body);
    http_response(http_response&& r);
    ~http_response();

    http_response& operator=(http_response&& r);

    operator bool() const { return curlCode == CURLE_OK; }

    inline const std::string& get_body() { return body; }

};

enum class http_method {
    GET, POST, PUT
};

class http_request {

public:
    typedef std::function<void(curl_off_t dltotal, curl_off_t dlnow, curl_off_t uptotal,
                               curl_off_t upnow)> progress_callback;
    typedef std::function<size_t(char* ptr, size_t size)> output_callback;

private:

    std::string url;
    std::string body;
    http_method method;
    std::map<std::string, std::string> headers;
    std::string user_agent;
    std::string encoding;
    long timeout = 10L;
    bool follow_location = false;
    progress_callback callback_progress;
    output_callback callback_output;

    static size_t curl_stringstream_write_func(void* ptr, size_t size, size_t nmemb, std::stringstream* s);
    static size_t curl_write_func(void* ptr, size_t size, size_t nmemb, http_request* s);
    static int curl_xferinfo(void* ptr, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow);

public:

    http_request() {}

    http_request(std::string url) : url(url) {}

    void set_url(const std::string& url) { this->url = url; }

    void set_body(const std::string& str) { this->body = str; }

    void set_body(const url_encoded_entity& ent);

    void set_gzip_body(const std::string& str);

    void set_method(http_method method) { this->method = method; }

    void add_header(const std::string& key, const std::string& value);

    void set_user_agent(const std::string& ua) { this->user_agent = ua; }

    void set_encoding(const std::string& encoding) { this->encoding = encoding; }

    void set_custom_output_func(output_callback callback) { this->callback_output = callback; }

    void set_timeout(long timeout) { this->timeout = timeout; }

    void set_follow_location(bool follow_location) { this->follow_location = follow_location; }

    void set_progress_callback(progress_callback callback) { this->callback_progress = callback; }

    http_response perform();

};

}