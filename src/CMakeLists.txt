cmake_minimum_required(VERSION 3.4)

# Compile flags
if (MSVC)
	add_compile_options("-Wall")
else()
	add_compile_options("-Wall" "-Wextra" "-Werror" "-Wno-implicit-fallthrough")
endif()


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

if (SCAFFOLD_BUILD_SHARED_LIB)
	message("Building scaffold as shared lib also (targetname - scaffold-shared)")
	add_library(scaffold-shared SHARED ${SOURCE_FILES})
endif()

target_include_directories(scaffold PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_include_directories(scaffold-shared PUBLIC ${PROJECT_SOURCE_DIR}/include)
