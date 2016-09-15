cmake_minimum_required(VERSION 3.4)
project(scaffold)

add_subdirectory(src)

# Tests
add_subdirectory(tests)

# Benchmark
option(ENABLE_BENCHMARKS "Build the benchmarks" off)
if (ENABLE_BENCHMARKS)
  add_subdirectory(bench)
endif()

