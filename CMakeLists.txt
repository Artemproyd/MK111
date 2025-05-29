cmake_minimum_required(VERSION 3.10)

project(SyntaxAnalyzer)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Force Windows to create console application
if(WIN32)
    set(CMAKE_WIN32_EXECUTABLE FALSE)
    # Ensure we link with filesystem on Windows
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static-libgcc -static-libstdc++")
endif()

# Add source files
set(SOURCES
    syntax_analyzer.cpp
    lexer.cpp
    stack_machine.cpp
    ops_generator.cpp
    ops_interpreter.cpp
)

# Add header files
set(HEADERS
    syntax_analyzer.h
    stack_machine.h
    ops_generator.h
    lexer.h
)

# Create executable
add_executable(syntax_analyzer ${SOURCES} ${HEADERS})

# Компиляторные флаги и предупреждения
if(MSVC)
    target_compile_options(syntax_analyzer PRIVATE /W4)
else()
    target_compile_options(syntax_analyzer PRIVATE -Wall -Wextra)
endif()

# Set output directories
set_target_properties(syntax_analyzer
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/../"
)

# Copy input.txt to output directory after build
add_custom_command(TARGET syntax_analyzer POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${CMAKE_SOURCE_DIR}/input.txt
    $<TARGET_FILE_DIR:syntax_analyzer>/input.txt
) 