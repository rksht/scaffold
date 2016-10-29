cmake_minimum_required(VERSION 3.4)

# Compile flags
add_compile_options("-Wall" "-Wextra" "-Werror")

# sanitizer
option(USE_MEMORY_SANITIZER "use memory sanitizer" off)
if (USE_MEMORY_SANITIZER)
	add_compile_options("-fsanitize=memory")
endif()


file(GLOB SOURCE_FILES
    *.cpp
    ${PROJECT_SOURCE_DIR}/include/scaffold/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/*.h)

add_library(scaffold ${SOURCE_FILES})

target_include_directories(scaffold PUBLIC ${PROJECT_SOURCE_DIR}/include)
