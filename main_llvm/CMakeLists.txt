cmake_minimum_required (VERSION 3.0)

include_directories(
    ${CMAKE_SOURCE_DIR}
)

add_executable(llvm_main main_llvm.cpp)

add_library(engine_llvm SHARED engine_llvm.cpp)
target_compile_options(engine_llvm PUBLIC -ggdb3)

target_link_libraries(llvm_main
    LLVM
    graphics
    engine_llvm
)
