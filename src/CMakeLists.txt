cmake_minimum_required(VERSION 3.4)

# Compile flags
add_compile_options("-std=c++14" "-Wall" "-Wextra" "-Werror")

file(GLOB SOURCE_FILES
    *.cpp
    ${PROJECT_SOURCE_DIR}/include/scaffold/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/*.h)

add_library(scaffold ${SOURCE_FILES})

include_directories(${PROJECT_SOURCE_DIR}/src)
target_include_directories(scaffold PUBLIC ${PROJECT_SOURCE_DIR}/include)


option(BUDDY_ALLOC_LEVEL_LOGGING "Log buddy allocator" off)

if(BUDDY_ALLOC_LEVEL_LOGGING)
  add_compile_options("-DBUDDY_ALLOC_LEVEL_LOGGING")
endif()


