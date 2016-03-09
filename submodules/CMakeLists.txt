add_subdirectory(seasocks)

add_custom_target(submodules)

add_dependencies(submodules seasocks)

include_directories("jeayeson/include")