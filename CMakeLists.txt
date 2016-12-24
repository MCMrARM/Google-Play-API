cmake_minimum_required(VERSION 3.6)
project(gplaydl)

set(CMAKE_CXX_STANDARD 11)

find_package(ZLIB REQUIRED)
find_package(CURL REQUIRED)
find_package(Protobuf REQUIRED)
include_directories(lib/)

protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS proto/gsf.proto)

set(LIB_UTIL_SOURCE_FILES lib/playapi/util/http.cpp lib/playapi/util/http.h lib/playapi/util/config.cpp lib/playapi/util/config.h lib/playapi/util/rand.cpp lib/playapi/util/rand.h lib/playapi/util/base64.cpp lib/playapi/util/base64.h)
set(LIB_SOURCE_FILES lib/playapi/login.cpp lib/playapi/login.h lib/playapi/device_info.cpp lib/playapi/device_info.h lib/playapi/checkin.cpp lib/playapi/checkin.h)
set(SOURCE_FILES src/main.cpp src/config.cpp src/config.h)
add_executable(gplaydl ${SOURCE_FILES} ${LIB_SOURCE_FILES} ${LIB_UTIL_SOURCE_FILES} ${PROTO_SRCS})
target_link_libraries(gplaydl ${CURL_LIBRARIES} ${ZLIB_LIBRARIES} ${Protobuf_LIBRARIES})
target_include_directories(gplaydl PUBLIC ${CURL_INCLUDE_DIR} ${ZLIB_INCLUDE_DIRS} ${Protobuf_INCLUDE_DIRS} ${CMAKE_CURRENT_BINARY_DIR})