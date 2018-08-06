cmake_minimum_required(VERSION 3.5)
project(gplaydl)

set(CMAKE_CXX_STANDARD 11)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake/")

find_package(Threads REQUIRED)
find_package(ZLIB REQUIRED)
find_package(CURL REQUIRED)
find_package(Protobuf REQUIRED)
find_package(LibUV REQUIRED)

if (NOT DEFINED Protobuf_LIBRARIES)
    set(Protobuf_LIBRARIES ${PROTOBUF_LIBRARIES})
    set(Protobuf_INCLUDE_DIRS ${PROTOBUF_INCLUDE_DIRS})
endif()

protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS proto/gsf.proto proto/play_common.proto proto/play_document.proto proto/play_respone.proto proto/play_settings.proto proto/play_toc.proto proto/play_download.proto proto/play_filter_rules.proto proto/play_ownership.proto proto/play_containers.proto proto/play_link.proto proto/play_device_config.proto proto/play_search.proto proto/play_browse.proto proto/play_details.proto)

set(LIB_UTIL_SOURCE_FILES lib/playapi/util/http.cpp include/playapi/util/http.h lib/playapi/util/config.cpp include/playapi/util/config.h lib/playapi/util/rand.cpp include/playapi/util/rand.h lib/playapi/util/base64.cpp include/playapi/util/base64.h)
set(LIB_SOURCE_FILES lib/playapi/login.cpp include/playapi/login.h lib/playapi/device_info.cpp include/playapi/device_info.h lib/playapi/checkin.cpp include/playapi/checkin.h include/playapi/api.h lib/playapi/api.cpp lib/playapi/experiments.cpp include/playapi/experiments.h include/playapi/login_cache.h include/playapi/file_login_cache.h lib/playapi/file_login_cache.cpp include/playapi/task.h include/playapi/util/http_request_pool.h lib/playapi/util/http_request_pool.cpp include/playapi/http_task.h)
set(SOURCE_FILES src/main.cpp src/config.cpp src/config.h)
add_library(gplayapi STATIC ${LIB_SOURCE_FILES} ${LIB_UTIL_SOURCE_FILES} ${PROTO_SRCS})
target_link_libraries(gplayapi ${CURL_LIBRARIES} ${ZLIB_LIBRARIES} ${Protobuf_LIBRARIES} ${LIBUV_LIBRARIES} Threads::Threads)
target_include_directories(gplayapi PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include ${CURL_INCLUDE_DIR} ${ZLIB_INCLUDE_DIRS} ${Protobuf_INCLUDE_DIRS} ${LIBUV_INCLUDE_DIRS} ${CMAKE_CURRENT_BINARY_DIR})

add_executable(gplaydl ${SOURCE_FILES})
target_link_libraries(gplaydl gplayapi)
target_include_directories(gplaydl PUBLIC ${CURL_INCLUDE_DIR} ${ZLIB_INCLUDE_DIRS})
