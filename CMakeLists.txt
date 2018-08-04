cmake_minimum_required (VERSION 3.5)

set(RINGBUFFER_LIBRARY_NAME ringbuffer)

set(META_PROJECT_NAME        ${RINGBUFFER_LIBRARY_NAME})
set(META_PROJECT_DESCRIPTION "Fast ring buffer for IO async processing")
set(META_AUTHOR_NAME         "Bogdan Gochev")
set(META_AUTHOR_DOMAIN       "http://bogdangochev.me")
set(META_AUTHOR_MAINTAINER   "bogdan.dgochev@gmail.com")
set(META_VERSION_MAJOR       "1")
set(META_VERSION_MINOR       "0")

project(${META_PROJECT_NAME} C CXX)

option(RINGBUFFER_TESTS "Enable tests" ON)
option(RINGBUFFER_BENCHMARKS "Enable benchmarks" ON)

########################################################## Global Variables :)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})

set(RINGBUFFER_SOURCE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/src")
set(RINGBUFFER_INCLUDE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/src")
set(RINGBUFFER_TEST_PATH "${CMAKE_CURRENT_SOURCE_DIR}/test")
set(RINGBUFFER_BENCHMARK_PATH "${CMAKE_CURRENT_SOURCE_DIR}/benchmark")

########################################################## Defaults

if (${CMAKE_CURRENT_SOURCE_DIR} STREQUAL ${CMAKE_CURRENT_BINARY_DIR})
	message(FATAL_ERROR "In-source builds not allowed. Please make a new directory (called a build directory)"
		"and run CMake from there. You may need to remove CMakeCache.txt.")
endif()


# default built type is Release
if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build" FORCE)
endif(NOT CMAKE_BUILD_TYPE)

########################################################## Threads

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package (Threads REQUIRED)

########################################################## Building Settings

include(cmake/BuildSettings.cmake)

########################################################## Building Ring buffer

add_subdirectory(${RINGBUFFER_SOURCE_PATH})

if(RINGBUFFER_TESTS)
	enable_testing()
	include(CTest)
	add_subdirectory(${RINGBUFFER_TEST_PATH})
endif()

if(RINGBUFFER_BENCHMARKS)
	add_subdirectory(${RINGBUFFER_BENCHMARK_PATH})
endif()