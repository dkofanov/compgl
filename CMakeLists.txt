cmake_minimum_required (VERSION 3.0)

find_package(OpenGL REQUIRED)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)

# Include third-party:
add_subdirectory(third_party)
add_subdirectory(graphics)
add_subdirectory(main)
add_subdirectory(main_llvm)

