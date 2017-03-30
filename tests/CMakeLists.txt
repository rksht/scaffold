cmake_minimum_required(VERSION 3.4)

add_compile_options("-Wall" "-Wextra")
include_directories(${PROJECT_SOURCE_DIR}/include ${CMAKE_CURRENT_SOURCE_DIR})

##

add_executable(fo_test fo_test.cpp)
target_link_libraries(fo_test scaffold)

##

add_executable(buddytestsmall buddytestsmall.cpp)
target_link_libraries(buddytestsmall scaffold)
add_executable(buddytest buddytest.cpp)
target_link_libraries(buddytest scaffold)

##

add_executable(buddytestbig buddytestbig.cpp)
target_link_libraries(buddytestbig scaffold)

##
add_executable(smallintarraytest smallintarraytest.cpp)
target_link_libraries(smallintarraytest scaffold)

##

add_executable(dysmallintarraytest dysmallintarraytest.cpp)
target_link_libraries(dysmallintarraytest scaffold)

##
add_executable(arena_test arena_test.cpp)
target_link_libraries(arena_test scaffold)

##
# add_executable(rbt_test rbt_test.cpp)
# rget_link_libraries(rbt_test scaffold)

##
add_executable(pod_hash_test pod_hash_test.cpp)
target_link_libraries(pod_hash_test scaffold)

##

add_executable(array_test array_test.cpp)
target_link_libraries(array_test scaffold)

##

add_executable(queue_test queue_test.cpp)
target_link_libraries(queue_test scaffold)

##

add_executable(buddy_array_test buddy_array_test.cpp)
target_link_libraries(buddy_array_test scaffold)

##

add_executable(pod_hash_test_2 pod_hash_test_2.cpp)
target_link_libraries(pod_hash_test_2 scaffold)

##

add_executable(temp_allocator_test temp_allocator_test.cpp)
target_link_libraries(temp_allocator_test scaffold)

if (USE_GPROF)
	SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pg")
	SET(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -pg")
	SET(CMAKE_STATIC_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -pg")
endif()
add_executable(pod_hash_gprof_test pod_hash_gprof_test.cpp)
target_link_libraries(pod_hash_gprof_test scaffold)
if (USE_GPROF)
	target_compile_options(pod_hash_gprof_test PRIVATE "-save-temps" "-pg")
endif()


##

add_executable(quad_hash_test quad_hash_test.cpp)
target_link_libraries(quad_hash_test scaffold)
