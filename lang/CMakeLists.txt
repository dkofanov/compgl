set(LANG_BINARY_DIR "${GLVM_BINARY_DIR}/lang")
set(LANG_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(GENERATED_DIR "${LANG_BINARY_DIR}/generated")
set(LEXER "lexer.l")
set(GRAMMAR "grammar.l")

add_custom_command(
    OUTPUT "${GENERATED_DIR}/${LEXER}.cpp" "${GENERATED_DIR}/${GRAMMAR}.cpp" "${GENERATED_DIR}/${GRAMMAR}.hpp"
    DEPENDS "${GENERATED_DIR}/${LEXER}" "${GENERATED_DIR}/${GRAMMAR}"
    COMMAND flex "-o" "${GENERATED_DIR}/${LEXER}.cpp" "${LANG_SOURCE_DIR}/${LEXER}"
    COMMAND bison "-o" "${GENERATED_DIR}/${GRAMMAR}.cpp" -d "${LANG_SOURCE_DIR}/${GRAMMAR}"
)

set(LANG_SRC 
    "${GENERATED_DIR}/${LEXER}.cpp"
    "${GENERATED_DIR}/${GRAMMAR}.cpp"
    lang_main.cpp
)
add_executable(lang_comp ${LANG_SRC})
target_include_directories(lang_comp PUBLIC ${GENERATED_DIR})
target_include_directories(lang_comp PUBLIC ${LANG_SOURCE_DIR})

target_link_libraries(lang_comp
    fl
    y
    LLVM
    graphics
)

target_compile_options(lang_comp PUBLIC -ggdb3)
