cmake_minimum_required(VERSION 3.4)

# Compile flags
add_compile_options("-std=c++14" "-Wall" "-Wextra" "-Werror")

# gprof
option(USE_GPROF "use gprof" off)
if (USE_GCOV)
	add_compile_options("-pg")
	SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pg")
	SET(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -pg")
	SET(CMAKE_STATIC_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -pg")
endif()


file(GLOB SOURCE_FILES
    *.cpp
    ${PROJECT_SOURCE_DIR}/include/scaffold/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/*.h)

add_library(scaffold ${SOURCE_FILES})

target_include_directories(scaffold PUBLIC ${PROJECT_SOURCE_DIR}/include)


option(BUDDY_ALLOC_LEVEL_LOGGING "Log buddy allocator" off)

if(BUDDY_ALLOC_LEVEL_LOGGING)
  add_compile_options("-DBUDDY_ALLOC_LEVEL_LOGGING")
endif()
