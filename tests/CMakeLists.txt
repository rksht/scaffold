cmake_minimum_required(VERSION 3.4)

add_compile_options("-std=c++14" "-Wall" "-Wextra")
include_directories(${PROJECT_SOURCE_DIR}/src)
include_directories(${PROJECT_SOURCE_DIR}/jeayeson/include)

##
option(BUDDY_ALLOC_LEVEL_LOGGING "Log buddy allocator" off)
add_compile_options("-std=c++14" "-Wall" "-Wextra")
if(BUDDY_ALLOC_LEVEL_LOGGING)
  message("Turning buddy logging on")
  add_compile_options("-DBUDDY_ALLOC_LEVEL_LOGGING")
endif()
add_executable(buddytestsmall buddytestsmall.cpp)
target_link_libraries(buddytestsmall scaffold)
add_executable(buddytest buddytest.cpp)
target_link_libraries(buddytest scaffold)
add_executable(buddytestbig buddytestbig.cpp)
target_link_libraries(buddytestbig scaffold)

##
add_executable(smallintarraytest smallintarraytest.cpp)
target_link_libraries(smallintarraytest scaffold)

##
add_executable(arena_test arena_test.cpp)
target_link_libraries(arena_test scaffold)

##
add_executable(rbt_test rbt_test.cpp)
target_link_libraries(rbt_test scaffold)

##
add_executable(pod_hash_test pod_hash_test.cpp)
target_link_libraries(pod_hash_test scaffold)

##
add_executable(hash_soa_test hash_soa_test.cpp)
target_link_libraries(hash_soa_test scaffold)

##

add_executable(array_test array_test.cpp)
target_link_libraries(array_test scaffold)

##

add_executable(pod_hash_test_2 pod_hash_test_2.cpp)
target_link_libraries(pod_hash_test_2 scaffold)
