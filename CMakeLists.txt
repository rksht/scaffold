cmake_minimum_required(VERSION 3.4)
project(scaffold)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(SCAFFOLD_ENABLE_STRING_ID_TABLE
  "Enable runtime checking of unique StringID using a map. Disabled on Release build"
  on)

# Benchmark
option(SCAFFOLD_ENABLE_BENCHMARKS "Build the benchmarks" off)
if (SCAFFOLD_ENABLE_BENCHMARKS)
  add_subdirectory(bench)
endif()

# Options to propagate
if (GNU OR Clang)
	add_compile_options(-std=c++17)
elseif(MSVC)
	add_compile_options(-std:c++17)
endif()

option(BUDDY_ALLOC_LEVEL_LOGGING "Log buddy allocator" off)
option(MALLOC_ALLOCATOR_DONT_TRACK_SIZE "Don't track size in MallocAllocator" off)

if(BUDDY_ALLOC_LEVEL_LOGGING)
  message("Turning level logging on")
  add_definitions(-DBUDDY_ALLOC_LEVEL_LOGGING)
endif()

if(MALLOC_ALLOCATOR_DONT_TRACK_SIZE)
  message("MallocAllocator will NOT track size")
  add_compile_options(-DMALLOC_ALLOCATOR_DONT_TRACK_SIZE=1)
else()
  add_compile_options(-DMALLOC_ALLOCATOR_DONT_TRACK_SIZE=0)
endif()

option(SCAFFOLD_USE_ASAN "Use address sanitizer" off)

if (${SCAFFOLD_USE_ASAN})
	add_compile_options(-fsanitize=address)
endif()

# Source
add_subdirectory(src)

option(SCAFFOLD_ENABLE_TESTS "Build the tests" off)

if (SCAFFOLD_ENABLE_TESTS)
	add_subdirectory(tests)
endif()
