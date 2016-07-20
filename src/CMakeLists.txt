cmake_minimum_required(VERSION 3.4)

set(SOURCE_FILES memory.cpp memory.h arena.cpp arena.h murmur_hash.cpp
  murmur_hash.h string_stream.cpp string_stream.h buddy.h scanner.cpp scanner.h
  pod_hash.h pod_hash_usuals.h pod_hash_usuals.cpp)

message("CMake Build Type = ${CMAKE_BUILD_TYPE}")
# Compile flags
option(BUDDY_ALLOC_LEVEL_LOGGING "Log buddy allocator" off)
add_compile_options("-std=c++14" "-Wall" "-Wextra" "-Wshadow")
if(BUDDY_ALLOC_LEVEL_LOGGING)
  add_compile_options("-DBUDDY_ALLOC_LEVEL_LOGGING")
endif()

include_directories(${PROJECT_SOURCE_DIR}/src)

add_library(scaffold ${SOURCE_FILES})
