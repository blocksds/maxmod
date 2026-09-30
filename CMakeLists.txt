# SPDX-License-Identifier: ISC
#
# Copyright (c) 2026, Antonio Niño Díaz

# This build system only builds the headless version, not GBA or NDS

cmake_minimum_required(VERSION 4.0)
project(maxmod)
enable_language(C)

enable_testing()

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_EXTENSIONS ON)
set(CMAKE_C_STANDARD_REQUIRED OFF)

if(EMSCRIPTEN)
    # On Web targets, we need CMake to generate a HTML file.
    set(CMAKE_EXECUTABLE_SUFFIX ".html" CACHE INTERNAL "")

    # Set the output directory for built objects.
    # This makes sure that the dynamic library goes into the build directory automatically.
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")

    add_subdirectory(3rdparty/SDL3 EXCLUDE_FROM_ALL)

    set(SDL3_FOUND TRUE)
else()
    find_package(SDL3)
endif()

if(SDL3_FOUND)
    set(BUILD_SDL3_PLAYER TRUE)
else()
    set(BUILD_SDL3_PLAYER FALSE)
endif()

if(EMSCRIPTEN)
    set(BUILD_WAV_RENDERER FALSE)
else()
    set(BUILD_WAV_RENDERER TRUE)
endif()

# Macros
# ------

# Compiler-specific options
macro(compiler_flags target)
    if(EMSCRIPTEN)
        # Nothing for now
    elseif(CMAKE_C_COMPILER_ID STREQUAL "GNU")
        target_compile_options(${target} PRIVATE
            -fwrapv
            -Wall -Wextra
        )
    endif()
endmacro()

# Macro that searches all the source files in the specified directory in 'dir'
# and saves them in 'var'
macro(search_source_files dir var)
    file(GLOB ${var} CONFIGURE_DEPENDS ${dir}/*.c ${dir}/*.h)
endmacro()

# Maxmod library
# --------------

add_library(maxmod)

search_source_files(source/core FILES_MAXMOD_SOURCE_CORE)
search_source_files(source/common FILES_MAXMOD_SOURCE_COMMON)
search_source_files(source/headless FILES_MAXMOD_SOURCE_HEADLESS)

target_compile_definitions(maxmod
    PRIVATE
        __HEADLESS__
)

target_include_directories(maxmod
    PRIVATE
        source
    PUBLIC
        include
)

target_sources(maxmod
    PRIVATE
        ${FILES_MAXMOD_SOURCE_CORE}
        ${FILES_MAXMOD_SOURCE_COMMON}
        ${FILES_MAXMOD_SOURCE_HEADLESS}
)

# Subdirectories
# --------------

add_subdirectory(tests)
add_subdirectory(tools)
