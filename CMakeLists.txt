cmake_minimum_required(VERSION 3.6)
project(gplaydl)

set(CMAKE_CXX_STANDARD 11)

find_package(ZLIB REQUIRED)
find_package(CURL REQUIRED)
find_package(Protobuf REQUIRED)
include_directories(lib/)

protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS proto/gsf.proto)

set(SOURCE_FILES src/main.cpp src/config.cpp src/config.h lib/playapi/login.cpp lib/playapi/login.h lib/playapi/httputils.cpp lib/playapi/httputils.h lib/playapi/device_info.cpp lib/playapi/device_info.h lib/playapi/config.cpp lib/playapi/config.h lib/playapi/checkin.cpp lib/playapi/checkin.h lib/playapi/rand.cpp lib/playapi/rand.h)
add_executable(gplaydl ${SOURCE_FILES} ${PROTO_SRCS})
target_link_libraries(gplaydl ${CURL_LIBRARIES} ${ZLIB_LIBRARIES} ${Protobuf_LIBRARIES})
target_include_directories(gplaydl PUBLIC ${CURL_INCLUDE_DIR} ${ZLIB_INCLUDE_DIRS} ${Protobuf_INCLUDE_DIRS} ${CMAKE_CURRENT_BINARY_DIR})