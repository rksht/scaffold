cmake_minimum_required(VERSION 3.4)
project(scaffold)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Options to propagate
if (GNU)
	add_compile_options(-std=c++17)
elseif(MSVC)
	add_compile_options(-std:c++17)
endif()

option(BUDDY_ALLOC_LEVEL_LOGGING "Log buddy allocator" off)
option(MALLOC_ALLOC_DONT_TRACK_SIZE "Don't track size in MallocAllocator" off)

if(BUDDY_ALLOC_LEVEL_LOGGING)
  message("Turning level logging on")
  add_definitions(-DBUDDY_ALLOC_LEVEL_LOGGING)
endif()

if(MALLOC_ALLOC_DONT_TRACK_SIZE)
  message("MallocAllocator will NOT track size")
  add_definitions(-DMALLOC_ALLOC_DONT_TRACK_SIZE)
endif()

# Source
add_subdirectory(src)

# Benchmark
option(SCAFFOLD_ENABLE_BENCHMARKS "Build the benchmarks" off)
if (SCAFFOLD_ENABLE_BENCHMARKS)
  add_subdirectory(bench)
endif()

option(SCAFFOLD_ENABLE_TESTS "Build the tests" off)

if (SCAFFOLD_ENABLE_TESTS)
	add_subdirectory(tests)
endif()
