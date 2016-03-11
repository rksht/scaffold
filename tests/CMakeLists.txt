cmake_minimum_required(VERSION 3.4)

add_compile_options("-std=c++14" "-ggdb" "-Wall" "-Wextra")

include_directories(${PROJECT_SOURCE_DIR}/src)
include_directories(${PROJECT_SOURCE_DIR}/submodules/jeayeson/include)
include_directories(${PROJECT_SOURCE_DIR}/submodules/seasocks/src/main/c)

add_executable(buddy_test buddytest.cpp)
target_link_libraries(buddy_test scaffold)
add_executable(smallintarraytest smallintarraytest.cpp)
target_link_libraries(smallintarraytest scaffold)
add_executable(seasocktest seasocktest.cpp)
target_link_libraries(seasocktest seasocks)
add_dependencies(seasocktest submodules)
