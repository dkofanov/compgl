cmake_minimum_required (VERSION 3.0)

set(CMAKE_CXX_COMPILER /usr/bin/clang++-14)
set(CMAKE_C_COMPILER /usr/bin/clang-14)
set(CMAKE_BUILD_TYPE Debug)
project(gllvm)

find_package(OpenGL REQUIRED)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)

# Include third-party:
add_subdirectory(third_party)
add_subdirectory(graphics)
add_subdirectory(main)
add_subdirectory(main_llvm)

