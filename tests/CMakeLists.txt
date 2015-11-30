cmake_minimum_required(VERSION 3.4)

add_compile_options("-std=c++14" "-O3" "-Wall" "-Wextra" "-Wshadow")

include_directories(${PROJECT_SOURCE_DIR}/src)

add_executable(rbt_test rbt_test.cpp)
add_dependencies(rbt_test scaffold)
target_link_libraries(rbt_test scaffold)