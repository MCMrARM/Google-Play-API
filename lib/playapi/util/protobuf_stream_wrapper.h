#pragma once

#include <google/protobuf/io/zero_copy_stream_impl_lite.h>

namespace playapi {

class stream;

namespace protobuf {

class ProtobufInputStreamWrapper : public google::protobuf::io::CopyingInputStream {

private:
    playapi::stream* stream;

public:
    explicit ProtobufInputStreamWrapper(playapi::stream* stream) : stream(stream) { }

    int Read(void* buffer, int size) override;

    int Skip(int count) override;


};

class ProtobufOutputStreamWrapper : public google::protobuf::io::CopyingOutputStream {

private:
    playapi::stream* stream;

public:
    explicit ProtobufOutputStreamWrapper(playapi::stream* stream) : stream(stream) { }

    bool Write(const void* buffer, int size) override;


};

}
}