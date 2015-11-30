cmake_minimum_required(VERSION 3.4)

project(scaffold)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# Main lib
add_subdirectory(src)

# Tests
add_subdirectory(tests)
