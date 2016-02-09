cmake_minimum_required(VERSION 3.4)

add_compile_options("-std=c++14" "-ggdb" "-Wall" "-Wextra" "-DNDEBUG")

include_directories(${PROJECT_SOURCE_DIR}/src)

#add_executable(rbt_test rbt_test.cpp)
#add_dependencies(rbt_test scaffold)
#target_link_libraries(rbt_test scaffold)

add_executable(buddy_test buddytest.cpp)
target_link_libraries(buddy_test scaffold)
add_executable(smallintarraytest smallintarraytest.cpp)
target_link_libraries(smallintarraytest scaffold)