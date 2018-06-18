# Finds the `uv` library.
# This file is released under the Public Domain.
# Once done this will define
#  LIBUV_FOUND - Set to true if evdev has been found
#  LIBUV_INCLUDE_DIRS - The evdev include directories
#  LIBUV_LIBRARIES - The libraries needed to use evdev

find_package(PkgConfig)
pkg_check_modules(PC_LIBUV QUIET libuv)

find_path(LIBUV_INCLUDE_DIR
        NAMES uv.h
        HINTS ${PC_LIBUV_INCLUDEDIR} ${PC_LIBUV_INCLUDE_DIRS})
find_library(LIBUV_LIBRARY
        NAMES uv libuv
        HINTS ${PC_LIBUV_LIBDIR} ${PC_LIBUV_LIBRARY_DIRS})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LIBUV DEFAULT_MSG
        LIBUV_LIBRARY LIBUV_INCLUDE_DIR)

mark_as_advanced(LIBUV_INCLUDE_DIR LIBUV_LIBRARY)

set(LIBUV_LIBRARIES ${LIBUV_LIBRARY})
set(LIBUV_INCLUDE_DIRS ${LIBUV_INCLUDE_DIR})