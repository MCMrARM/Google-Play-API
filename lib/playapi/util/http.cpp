#include <playapi/util/http.h>

#include <cassert>
#include <stdexcept>
#include <zlib.h>
#include <thread>
#include <memory>

using namespace playapi;

void url_encoded_entity::add_pair(const std::string& key, const std::string& val) {
    pairs.push_back({key, val});
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

http_response::http_response(CURLcode curlCode, long statusCode, std::string body) :
        curlCode(curlCode), statusCode(statusCode), body(std::move(body)) {
    //
}

http_response::http_response(http_response&& r) : curlCode(r.curlCode), statusCode(r.statusCode),
                                                  body(r.body) {
    r.curlCode = CURLE_FAILED_INIT;
    r.statusCode = 0;
    r.body = std::string();
}

http_response& http_response::operator=(http_response&& r) {
    curlCode = r.curlCode;
    body = r.body;
    r.curlCode = CURLE_FAILED_INIT;
    r.body = std::string();
    return *this;
}

http_response::~http_response() {
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

size_t http_request::curl_stringstream_write_func(void* ptr, size_t size, size_t nmemb, std::stringstream* s) {
    s->write((char*) ptr, size * nmemb);
    return size * nmemb;
}
size_t http_request::curl_write_func(void* ptr, size_t size, size_t nmemb, output_callback* s) {
    return (*s)((char*) ptr, size * nmemb);
}
int http_request::curl_xferinfo(void* ptr, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow) {
    progress_callback* req = (progress_callback*) ptr;
    (*req)(dltotal, dlnow, ultotal, ulnow);
    return 0;
}

CURL* http_request::build(std::stringstream& output, bool copy_body) {
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
        curl_easy_setopt(curl, copy_body ? CURLOPT_COPYPOSTFIELDS : CURLOPT_POSTFIELDS, body.c_str());
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
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeout);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, follow_location ? 1L : 0L);

#ifndef NDEBUG
    printf("http request: %s, body = %s\n", url.c_str(), body.c_str());
#endif
    if (callback_output) {
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &callback_output);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_func);
    } else {
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &output);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_stringstream_write_func);
    }
    if (callback_progress) {
        curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, curl_xferinfo);
        curl_easy_setopt(curl, CURLOPT_XFERINFODATA, &callback_progress);
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
    }
    return curl;
}

http_response http_request::perform() {
    std::stringstream output;
    CURL* curl = build(output);
    CURLcode ret = curl_easy_perform(curl);
    long status;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status);
#ifndef NDEBUG
    printf("http response body: %s\n", output.str().c_str());
#endif
    curl_easy_cleanup(curl);
    return http_response(ret, status, output.str());
}

void http_request::perform(std::function<void(http_response)> success, std::function<void(std::exception_ptr)> error) {
    auto req = std::make_shared<http_request>(*this);
    std::thread([req, success, error]() {
        std::stringstream output;
        CURL* curl = req->build(output);
        char errbuf[CURL_ERROR_SIZE];
        curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, errbuf);
        if (req->callback_output) {
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &req->callback_output);
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_func);
        }
        if (req->callback_progress) {
            curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, curl_xferinfo);
            curl_easy_setopt(curl, CURLOPT_XFERINFODATA, &req->callback_progress);
            curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
        }
        CURLcode curlerr = curl_easy_perform(curl);
        if (curlerr == CURLE_OK) {
            long status;
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status);
    #ifndef NDEBUG
            printf("http response body: %s\n", output.str().c_str());
    #endif
            success(http_response(curlerr, status, output.str()));
        } else {
            std::stringstream errormsg;
            errormsg << "Failed to perform http request to " << req->url << " : CURLcode " << curlerr << " Details: " << errbuf;
            try {
                throw std::runtime_error(errormsg.str().data());
            } catch (...) {
                error(std::current_exception());
            }
        }
        curl_easy_cleanup(curl);
    }).detach();
}
