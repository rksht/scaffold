cmake_minimum_required(VERSION 3.4)

set(CMAKE_CXX_COMPILER clang++)

project(scaffold)

# Main lib
add_subdirectory(src)

# Tests
add_subdirectory(tests)
