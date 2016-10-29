cmake_minimum_required(VERSION 3.4)
project(scaffold)

# Options to propagate

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(BUDDY_ALLOC_LEVEL_LOGGING)
  messge("Turning level logging on")
  add_definitions(-DBUDDY_ALLOC_LEVEL_LOGGING)
endif()

if(MALLOC_ALLOC_DONT_TRACK_SIZE)
  message("MallocAllocator will NOT track size")
  add_definitions(-DMALLOC_ALLOC_DONT_TRACK_SIZE)
endif()

# Source
add_subdirectory(src)

# Tests
add_subdirectory(tests)

# Benchmark
option(ENABLE_BENCHMARKS "Build the benchmarks" off)
if (ENABLE_BENCHMARKS)
  add_subdirectory(bench)
endif()

option(BUDDY_ALLOC_LEVEL_LOGGING "Log buddy allocator" off)
option(MALLOC_ALLOC_DONT_TRACK_SIZE "Don't track size in MallocAllocator" off)
