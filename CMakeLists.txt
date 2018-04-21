cmake_minimum_required (VERSION 3.6)

set(META_PROJECT_NAME        "ringbuffer")
set(META_PROJECT_DESCRIPTION "Fast ring buffer for IO async processing")
set(META_AUTHOR_NAME         "Bogdan Gochev")
set(META_AUTHOR_DOMAIN       "http://bogdangochev.me")
set(META_AUTHOR_MAINTAINER   "bogdan.dgochev@gmail.com")
set(META_VERSION_MAJOR       "1")
set(META_VERSION_MINOR       "0")

project(${META_PROJECT_NAME} C CXX)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})

include(cmake/BuildSettings.cmake)

set(CUSTOM_INCLUDE_DIRECTORIES)
set(ringbufferLibaryName ringbuffer)

add_subdirectory(src)