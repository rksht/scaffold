cmake_minimum_required(VERSION 3.4)

set(CMAKE_CXX_COMPILER clang++)

project(Scaffold)

# Main lib
add_subdirectory(src)

# Tests
add_subdirectory(tests)

# Submodules
#add_subdirectory(submodules)

add_subdirectory(seasocks)

