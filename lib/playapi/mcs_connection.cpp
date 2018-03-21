#include "mcs_connection.h"

#include <google/protobuf/io/coded_stream.h>
#include <mcs.pb.h>
#include "util/protobuf_stream_wrapper.h"
#include "device_info.h"
#include "checkin.h"

using namespace playapi;
using namespace playapi::protobuf;
using namespace google::protobuf::io;

const char* const mcs_connection::SERVICE_HOSTNAME = "mtalk.google.com";
unsigned short const mcs_connection::SERVICE_PORT = 5228;
unsigned char mcs_connection::SERVICE_VERSION = 41;

std::unique_ptr<google::protobuf::MessageLite> playapi::mcs_message_for_tag(mcs_proto_tag tag) {
    switch (tag) {
        case mcs_proto_tag::login_response:
            return std::unique_ptr<google::protobuf::MessageLite>(new proto::mcs::LoginResponse);
        case mcs_proto_tag::iq_stanza:
            return std::unique_ptr<google::protobuf::MessageLite>(new proto::mcs::IqStanza);
        case mcs_proto_tag::stream_error_stanza:
            return std::unique_ptr<google::protobuf::MessageLite>(new proto::mcs::StreamErrorStanza);
        default:
            throw std::runtime_error("Got invalid message tag: " + std::to_string((int) tag));
    }
}

mcs_connection::mcs_connection() {
    //
}

void mcs_connection::connect() {
    conn_socket = std::unique_ptr<socket>(new socket(SERVICE_HOSTNAME, SERVICE_PORT));
    conn = std::unique_ptr<stream>(new ssl_socket(*conn_socket));
    ProtobufInputStreamWrapper* input_wrapper = new ProtobufInputStreamWrapper(conn.get());
    protobuf_stream_in = std::unique_ptr<CopyingInputStreamAdaptor>(new CopyingInputStreamAdaptor(input_wrapper));
    ProtobufOutputStreamWrapper* output_wrapper = new ProtobufOutputStreamWrapper(conn.get());
    protobuf_stream_out = std::unique_ptr<CopyingOutputStreamAdaptor>(new CopyingOutputStreamAdaptor(output_wrapper));
}

void mcs_connection::send_login_request(checkin_result const& checkin_info) {
    proto::mcs::LoginRequest req;
    req.set_id("gms-12.2.21-000");
    req.set_domain("mcs.android.com");
    req.set_user(std::to_string(checkin_info.android_id));
    req.set_resource(std::to_string(checkin_info.android_id));
    req.set_auth_token(std::to_string(checkin_info.security_token));
    req.set_device_id("android-" + checkin_info.get_string_android_id());
    req.set_use_rmq2(true);
    req.set_auth_service(proto::mcs::LoginRequest::ANDROID_ID);
    req.set_network_type(1);

    {
        mcs_proto_tag tag = mcs_proto_tag::login_request;
        CodedOutputStream coded_output_stream(protobuf_stream_out.get());
        coded_output_stream.WriteRaw(&SERVICE_VERSION, 1);
        coded_output_stream.WriteRaw(&tag, 1);
        coded_output_stream.WriteVarint32((unsigned int) req.ByteSize());
        req.SerializeToCodedStream(&coded_output_stream);
        printf("Send login req: %s\n", req.DebugString().c_str());
    }
    protobuf_stream_out->Flush();
}

void mcs_connection::send_message(mcs_proto_tag tag, google::protobuf::MessageLite& message) {
    assert(handshake_complete);
    {
        CodedOutputStream coded_output_stream(protobuf_stream_out.get());
        coded_output_stream.WriteRaw(&tag, 1);
        coded_output_stream.WriteVarint32((unsigned int) message.ByteSize());
        message.SerializeToCodedStream(&coded_output_stream);
    }
    protobuf_stream_out->Flush();
}

void mcs_connection::handle_incoming() {
    CodedInputStream coded_input_stream(protobuf_stream_in.get());
    while (true) {
        if (!got_server_version) {
            unsigned char version;
            coded_input_stream.ReadRaw(&version, 1);
            if (version < SERVICE_VERSION && version != 38)
                throw std::runtime_error("Got invalid MCS version " + std::to_string(version));
            got_server_version = true;
        }

        mcs_proto_tag tag = (mcs_proto_tag) 0;
        unsigned int size;

        coded_input_stream.ReadRaw(&tag, 1);
        coded_input_stream.ReadVarint32(&size);

        auto message = mcs_message_for_tag(tag);
        auto limit = coded_input_stream.PushLimit(size);
        if (!message->ParsePartialFromCodedStream(&coded_input_stream)) // this fails for wtf reason
            throw std::runtime_error("");
        assert(coded_input_stream.ConsumedEntireMessage());
        coded_input_stream.PopLimit(limit);

        printf("MCS [tag=%i]\n%s\n\n", (int) tag, ((google::protobuf::Message*) message.get())->DebugString().c_str());
    }
}