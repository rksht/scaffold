cmake_minimum_required(VERSION 3.4)

set(SOURCE_FILES memory.cpp memory.h murmur_hash.cpp murmur_hash.h
  string_stream.cpp string_stream.h buddy.h scanner.cpp scanner.h pod_hash.h
  pod_hash_usuals.h pod_hash_usuals.cpp)

add_compile_options("-std=c++14" "-ggdb" "-Wall" "-Wextra" "-Wshadow")

include_directories(${PROJECT_SOURCE_DIR}/src)

include_directories(${PROJECT_SOURCE_DIR}/submodules/jeayeson/include)

include_directories(${PROJECT_SOURCE_DIR}/submodules/seasocks/src/main/c)

add_library(scaffold ${SOURCE_FILES})
target_link_libraries(scaffold seasocks)
add_dependencies(scaffold submodules)
