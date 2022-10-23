cmake_minimum_required (VERSION 3.0)

include_directories(
    ../graphics/
)
# Main
add_executable(main main.c)
target_link_libraries(main
    graphics
)
