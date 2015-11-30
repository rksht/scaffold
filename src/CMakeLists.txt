cmake_minimum_required(VERSION 3.4)

set(SOURCE_FILES memory.cpp json.cpp murmur_hash.cpp scanner.cpp string_stream.cpp sds.cpp)

add_compile_options("-std=c++14" "-O3" "-Wall" "-Wextra" "-Wshadow")

include_directories(${PROJECT_SOURCE_DIR}/src)

add_library(scaffold ${SOURCE_FILES})
