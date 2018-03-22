#pragma once

#include <memory>
#include <google/protobuf/message_lite.h>
#include <google/protobuf/io/zero_copy_stream_impl_lite.h>
#include "util/ssl_socket.h"

namespace playapi {

enum class mcs_proto_tag {
    heartbeat_ping = 0,
    Heartbeat_ack,
    login_request,
    login_response,
    close,
    message_stanza,
    presence_stanza,
    iq_stanza,
    data_message_stanza,
    batch_presence_stanza,
    stream_error_stanza,
    http_request,
    http_response,
    bind_account_request,
    bind_account_response,
    talk_metadata,

    num_proto_types
};

std::unique_ptr<google::protobuf::MessageLite> mcs_message_for_tag(mcs_proto_tag tag);

class device_info;
struct checkin_result;
class login_api;
class mcs_registration_api;

class mcs_connection {

private:

    static const char* const SERVICE_HOSTNAME;
    static unsigned short const SERVICE_PORT;
    static unsigned char SERVICE_VERSION;

    std::unique_ptr<socket> conn_socket;
    std::unique_ptr<stream> conn;
    std::unique_ptr<google::protobuf::io::CopyingInputStreamAdaptor> protobuf_stream_in;
    std::unique_ptr<google::protobuf::io::CopyingOutputStreamAdaptor> protobuf_stream_out;
    bool handshake_complete = false;
    bool got_server_version = false;

public:

    mcs_connection();

    void connect();

    void send_login_request(checkin_result const& checkin_info);

    void send_message(mcs_proto_tag tag, google::protobuf::MessageLite& message);

    void handle_incoming();

    void bind_account(login_api& login, mcs_registration_api& registration_api);

};

}