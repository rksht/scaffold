cmake_minimum_required(VERSION 3.4)

# Compile flags
if(${MSVC})
  add_compile_options("-Wall")
else()
  add_compile_options("-Wall" "-Wextra" "-Werror" "-Wno-implicit-fallthrough")
endif()

# sanitizer
option(USE_MEMORY_SANITIZER "use memory sanitizer" off)
if(USE_MEMORY_SANITIZER)
  add_compile_options("-fsanitize=memory")
endif()

file(GLOB SOURCE_FILES
          *.cpp
          ${PROJECT_SOURCE_DIR}/include/scaffold/*.h
          ${CMAKE_CURRENT_SOURCE_DIR}/*.h)

if (${BUILD_SHARED_LIBS})
  message("Building scaffold as a shared library")
  if (${MSVC})
    add_compile_options("-DSCAFFOLD_API=__declspec(dllexport)")
  else()
    add_compile_options("-DSCAFFOLD_API=__attribute__((visibility(\"default\")))")
  endif()

  add_library(scaffold SHARED ${SOURCE_FILES})
else()
  add_library(scaffold ${SOURCE_FILES})
endif()

target_compile_options(scaffold PRIVATE "-DBUILDING_DLL=1" "-fvisibility=hidden")
target_include_directories(scaffold PUBLIC ${PROJECT_SOURCE_DIR}/include)
