include_directories(
    ../third_party/glfw-3.1.2/include/
    ../third_party/glew-1.13.0/include/
)

add_library(graphics SHARED graphics.c shader.cpp)
target_link_libraries(graphics
    ${OPENGL_LIBRARY}
    glfw
    GLEW_1130
)
