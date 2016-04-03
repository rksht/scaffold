cmake_minimum_required(VERSION 3.4)

add_compile_options("-std=c++14" "-ggdb" "-Wall" "-Wextra" "-DNDEBUG")

include_directories(${PROJECT_SOURCE_DIR}/src)
#include_directories(${PROJECT_SOURCE_DIR}/submodules/jeayeson/include)
#include_directories(${PROJECT_SOURCE_DIR}/submodules/seasocks/src/main/c)
include_directories(${PROJECT_SOURCE_DIR}/jeayeson/include)
include_directories(${PROJECT_SOURCE_DIR}/seasocks/src/main/c)
#add_executable(buddy_test buddytest.cpp)
add_executable(buddytestsmall buddytestsmall.cpp)
target_link_libraries(buddytestsmall scaffold)
add_executable(buddytest buddytest.cpp)
target_link_libraries(buddytest scaffold)
add_executable(smallintarraytest smallintarraytest.cpp)
target_link_libraries(smallintarraytest scaffold)
add_executable(rbt_test rbt_test.cpp)
target_link_libraries(rbt_test scaffold)


