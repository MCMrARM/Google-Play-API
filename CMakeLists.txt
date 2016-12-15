cmake_minimum_required(VERSION 3.6)
project(gplaydl)

set(CMAKE_CXX_STANDARD 11)

find_package(ZLIB REQUIRED)
find_package(CURL REQUIRED)
find_package(Protobuf REQUIRED)
include_directories(lib/)

protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS proto/gsf.proto)

set(SOURCE_FILES src/main.cpp src/config.cpp src/config.h lib/playdl/login.cpp lib/playdl/login.h lib/playdl/httputils.cpp lib/playdl/httputils.h lib/playdl/device_info.cpp lib/playdl/device_info.h lib/playdl/config.cpp lib/playdl/config.h lib/playdl/checkin.cpp lib/playdl/checkin.h lib/playdl/rand.cpp lib/playdl/rand.h)
add_executable(gplaydl ${SOURCE_FILES} ${PROTO_SRCS})
target_link_libraries(gplaydl ${CURL_LIBRARIES} ${ZLIB_LIBRARIES} ${Protobuf_LIBRARIES})
target_include_directories(gplaydl PUBLIC ${CURL_INCLUDE_DIR} ${ZLIB_INCLUDE_DIRS} ${Protobuf_INCLUDE_DIRS} ${CMAKE_CURRENT_BINARY_DIR})