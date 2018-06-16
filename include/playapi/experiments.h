#pragma once

#include <set>
#include <vector>
#include <play_respone.pb.h>

namespace playapi {

class http_request;

class experiments_list {

public:

    static const long long ENCODED_TARGETS = 12610177;

    static std::set<long long> supported_experiments;

    std::set<long long> enabled_experiments;
    std::set<long long> other_experiments;

    bool is_enabled(long long experiment) const;

    void set_targets(const proto::finsky::response::Targets& experiments);

    void set_targets(const std::string& experiments);

    void add_headers(http_request& req);

    std::string get_comma_separated_target_list() const;

};


}