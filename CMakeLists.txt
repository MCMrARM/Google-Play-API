cmake_minimum_required(VERSION 3.6)
project(gplaydl)

set(CMAKE_CXX_STANDARD 11)

find_package(ZLIB REQUIRED)
find_package(CURL REQUIRED)
find_package(Protobuf REQUIRED)
find_package(OpenSSL REQUIRED)

protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS proto/gsf.proto proto/play_common.proto proto/play_document.proto proto/play_respone.proto proto/play_settings.proto proto/play_toc.proto proto/play_download.proto proto/play_filter_rules.proto proto/play_ownership.proto proto/play_containers.proto proto/play_link.proto proto/play_device_config.proto proto/play_search.proto proto/play_browse.proto proto/play_details.proto)

set(LIB_UTIL_SOURCE_FILES lib/playapi/util/http.cpp lib/playapi/util/http.h lib/playapi/util/config.cpp lib/playapi/util/config.h lib/playapi/util/rand.cpp lib/playapi/util/rand.h lib/playapi/util/base64.cpp lib/playapi/util/base64.h lib/playapi/util/stream.h lib/playapi/util/socket.cpp lib/playapi/util/socket.h lib/playapi/util/ssl_socket.cpp lib/playapi/util/ssl_socket.h)
set(LIB_SOURCE_FILES lib/playapi/login.cpp lib/playapi/login.h lib/playapi/device_info.cpp lib/playapi/device_info.h lib/playapi/checkin.cpp lib/playapi/checkin.h lib/playapi/api.h lib/playapi/api.cpp lib/playapi/experiments.cpp lib/playapi/experiments.h)
set(SOURCE_FILES src/main.cpp src/config.cpp src/config.h)
add_library(gplayapi STATIC ${LIB_SOURCE_FILES} ${LIB_UTIL_SOURCE_FILES} ${PROTO_SRCS})
target_link_libraries(gplayapi ${CURL_LIBRARIES} ${ZLIB_LIBRARIES} ${Protobuf_LIBRARIES} ${OPENSSL_LIBRARIES})
target_include_directories(gplayapi PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/lib ${CURL_INCLUDE_DIR} ${ZLIB_INCLUDE_DIRS} ${OPENSSL_INCLUDE_DIRS} ${Protobuf_INCLUDE_DIRS} ${CMAKE_CURRENT_BINARY_DIR})

add_executable(gplaydl ${SOURCE_FILES})
target_link_libraries(gplaydl gplayapi)
target_include_directories(gplaydl PUBLIC ${CURL_INCLUDE_DIR} ${ZLIB_INCLUDE_DIRS} ${OPENSSL_INCLUDE_DIRS})