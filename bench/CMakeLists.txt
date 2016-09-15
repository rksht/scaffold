cmake_minimum_required(VERSION 3.4)

add_subdirectory(benchmark)

add_compile_options("-std=c++14" "-Wall" "-Wextra")

include_directories(${PROJECT_SOURCE_DIR}/include)

add_executable(hash_bench hash_bench.cpp)
target_link_libraries(hash_bench benchmark scaffold)
