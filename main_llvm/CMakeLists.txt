cmake_minimum_required (VERSION 3.0)

include_directories(
    ../graphics/
)
# Main
add_executable(llvm_main main_llvm.cpp)
target_link_libraries(llvm_main
    /usr/lib/llvm-10/lib
    LLVM-10
    graphics
)
add_compile_options(
    -I/usr/lib/llvm-10/include -std=c++14 -fno-exceptions 
    -D_GNU_SOURCE -D__STDC_CONSTANT_MACROS -D__STDC_FORMAT_MACROS -D__STDC_LIMIT_MACROS
    -L/usr/lib/llvm-10/lib 
    -lLLVM-10
)

add_executable(llvm_demo DEMO_LLVM.cpp)
target_link_libraries(llvm_demo
    /usr/lib/llvm-10/lib
    LLVM-10
    graphics
)
add_compile_options(
    -I/usr/lib/llvm-10/include -std=c++14 -fno-exceptions 
    -D_GNU_SOURCE -D__STDC_CONSTANT_MACROS -D__STDC_FORMAT_MACROS -D__STDC_LIMIT_MACROS
    -L/usr/lib/llvm-10/lib 
    -lLLVM-10
)
