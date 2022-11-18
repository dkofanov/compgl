add_definitions(
    -DTW_STATIC
    -DTW_NO_LIB_PRAGMA
    -DTW_NO_DIRECT3D
    -DGLEW_STATIC
    -D_CRT_SECURE_NO_WARNINGS
)

### GLFW ###

add_subdirectory (glfw-3.1.2)

include_directories(SYSTEM 
    ${CMAKE_CURRENT_SOURCE_DIR}/glfw-3.1.2/include/
    ${CMAKE_CURRENT_SOURCE_DIR}/glew-1.13.0/include/
)

if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
set(OPENGL_LIBRARY
    ${OPENGL_LIBRARY}
    -lGL -lGLU -lXrandr -lXext -lX11 -lrt
    ${CMAKE_DL_LIBS}
    ${GLFW_LIBRARIES}
)
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
set(OPENGL_LIBRARY
    ${OPENGL_LIBRARY}
    ${CMAKE_DL_LIBS}
    ${GLFW_LIBRARIES}
)
endif()

### GLEW ###

set(GLEW_SOURCE
    glew-1.13.0/src/glew.c
)

set(GLEW_HEADERS
)


add_library( GLEW_1130 SHARED
    ${GLEW_SOURCE}
    ${GLEW_INCLUDE}
)

target_link_libraries(GLEW_1130
    ${OPENGL_LIBRARY}
    ${EXTRA_LIBS}
)

### LLVM ###
# Enable if there is a need to build llvm from scratch

#[[
set(LLVM_BUILD_LLVM_DYLIB true)
add_subdirectory(llvm-project/llvm)
include_directories(SYSTEM
    ${CMAKE_CURRENT_SOURCE_DIR}/llvm-project/llvm/include/llvm
)

set(LLVM_DIR ${CMAKE_BINARY_DIR}/third_party/llvm-project/llvm)
#find_package(LLVM REQUIRED CONFIG PATHS ${LLVM_DIR} NO_DEFAULT_PATH)
message(LLVM_INCLUDE_DIRS: ${LLVM_INCLUDE_DIRS})
message(LLVM_LIBRARY_DIRS: ${LLVM_LIBRARY_DIRS})
message(LLVM_DEFINITIONS: ${LLVM_DEFINITIONS})
message(LLVM_LIBRARIES: ${LLVM_LIBRARIES})
#]]
