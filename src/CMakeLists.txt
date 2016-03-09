cmake_minimum_required(VERSION 3.4)

set(SOURCE_FILES memory.cpp json.cpp murmur_hash.cpp scanner.cpp string_stream.cpp sds.cpp)

add_compile_options("-std=c++14" "-ggdb" "-Wall" "-Wextra" "-Wshadow" "-DNDEBUG")

include_directories(${PROJECT_SOURCE_DIR}/src)

include_directories(${PROJECT_SOURCE_DIR}/submodules/jeayeson/include)

include_directories(${PROJECT_SOURCE_DIR}/src/submodules/seasocks/src/main/c)

add_library(scaffold ${SOURCE_FILES})
target_link_libraries(scaffold seasocks)
add_dependencies(scaffold submodules)