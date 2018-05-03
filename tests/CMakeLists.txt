cmake_minimum_required(VERSION 3.4)

if (MSVC)
	add_compile_options("-Wall")
else()
	add_compile_options("-Wall" "-Wextra" "-Wno-implicit-fallthrough"
      "-fmax-errors=1")
endif()

add_compile_options(-DSOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")

add_executable(single_test single_test.cpp)
target_link_libraries(single_test scaffold)

set_target_properties(single_test PROPERTIES FOLDER scaffold_tests)

include_directories(${PROJECT_SOURCE_DIR}/include ${CMAKE_CURRENT_SOURCE_DIR})

##

add_executable(fo_test fo_test.cpp)
target_link_libraries(fo_test scaffold)

set_target_properties(fo_test PROPERTIES FOLDER scaffold_tests)

##

add_executable(buddytestsmall buddytestsmall.cpp)
target_link_libraries(buddytestsmall scaffold)
set_target_properties(buddytestsmall PROPERTIES FOLDER scaffold_tests)

add_executable(buddytest buddytest.cpp)
target_link_libraries(buddytest scaffold)

set_target_properties(buddytest PROPERTIES FOLDER scaffold_tests)

##

add_executable(buddytestbig buddytestbig.cpp)
target_link_libraries(buddytestbig scaffold)

set_target_properties(buddytestbig PROPERTIES FOLDER scaffold_tests)

##
add_executable(packeduintarraytest packeduintarraytest.cpp)
target_link_libraries(packeduintarraytest scaffold)

set_target_properties(packeduintarraytest PROPERTIES FOLDER scaffold_tests)

##

add_executable(dypackeduintarraytest dypackeduintarraytest.cpp)
target_link_libraries(dypackeduintarraytest scaffold)

set_target_properties(dypackeduintarraytest PROPERTIES FOLDER scaffold_tests)

##

add_executable(arena_test arena_test.cpp)
target_link_libraries(arena_test scaffold)

set_target_properties(arena_test PROPERTIES FOLDER scaffold_tests)

##

add_executable(pod_hash_test pod_hash_test.cpp)
target_link_libraries(pod_hash_test scaffold)

set_target_properties(pod_hash_test PROPERTIES FOLDER scaffold_tests)

##

add_executable(array_test array_test.cpp)
target_link_libraries(array_test scaffold)

set_target_properties(array_test PROPERTIES FOLDER scaffold_tests)

##

add_executable(queue_test queue_test.cpp)
target_link_libraries(queue_test scaffold)

set_target_properties(queue_test PROPERTIES FOLDER scaffold_tests)

##

add_executable(buddy_array_test buddy_array_test.cpp)
target_link_libraries(buddy_array_test scaffold)

set_target_properties(buddy_array_test PROPERTIES FOLDER scaffold_tests)

##

add_executable(pod_hash_test_2 pod_hash_test_2.cpp)
target_link_libraries(pod_hash_test_2 scaffold)

set_target_properties(pod_hash_test_2 PROPERTIES FOLDER scaffold_tests)

##

add_executable(temp_allocator_test temp_allocator_test.cpp)
target_link_libraries(temp_allocator_test scaffold)

set_target_properties(temp_allocator_test PROPERTIES FOLDER scaffold_tests)

##

add_executable(open_hash_test open_hash_test.cpp)
target_link_libraries(open_hash_test scaffold)

set_target_properties(open_hash_test PROPERTIES FOLDER scaffold_tests)

## 

add_executable(pool_allocator_test pool_allocator_test.cpp)
target_link_libraries(pool_allocator_test scaffold)

set_target_properties(pool_allocator_test PROPERTIES FOLDER scaffold_tests)

##

add_executable(rbt_test_new rbt_test_new.cpp)
target_link_libraries(rbt_test_new scaffold)

set_target_properties(rbt_test_new PROPERTIES FOLDER scaffold_tests)

add_executable(non_pod_array_test non_pod_array_test.cpp)
target_link_libraries(non_pod_array_test scaffold)

set_target_properties(non_pod_array_test PROPERTIES FOLDER scaffold_tests)
