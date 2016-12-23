#pragma once

#include <string>
#include <map>
#include <curl/curl.h>

namespace playapi {

class http_request;

class url_encoded_entity {

public:

    std::map<std::string, std::string> pairs;

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

private:

    std::string url;
    std::string body;
    http_method method;
    std::map<std::string, std::string> headers;
    std::string user_agent;
    std::string encoding;

public:

    http_request() { }

    http_request(std::string url) : url(url) { }

    void set_url(const std::string& url) { this->url = url; }

    void set_body(const std::string& str) { this->body = str; }

    void set_body(const url_encoded_entity& ent);

    void set_gzip_body(const std::string& str);

    void set_method(http_method method) { this->method = method; }

    void add_header(const std::string& key, const std::string& value);

    void set_user_agent(const std::string& ua) { this->user_agent = ua; }

    void set_encoding(const std::string& encoding) { this->encoding = encoding; }

    http_response perform();

};

}