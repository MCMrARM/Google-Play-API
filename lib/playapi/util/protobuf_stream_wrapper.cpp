#include "protobuf_stream_wrapper.h"

#include "stream.h"

using namespace playapi::protobuf;

int ProtobufInputStreamWrapper::Read(void* buffer, int size) {
    return (int) stream->read(buffer, (size_t) size);
}

int ProtobufInputStreamWrapper::Skip(int) {
    throw std::runtime_error("Unsupported operation (Skip)");
}

bool ProtobufOutputStreamWrapper::Write(const void* buffer, int size) {
    return stream->write_full(buffer, (size_t) size);
}