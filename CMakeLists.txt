# SPDX-License-Identifier: ISC
#
# Copyright (c) 2026, Antonio Niño Díaz

# This build system only builds the headless version, not GBA or NDS

cmake_minimum_required(VERSION 4.0)
project(maxmod)
enable_language(C)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Macros
# ------

# Compiler-specific options
macro(compiler_flags target)
    if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
        target_compile_options(${target} PRIVATE
            -fwrapv
            -Wall -Wextra
        )
    elseif(CMAKE_C_COMPILER_ID STREQUAL "MSVC")
        target_compile_options(${target}  PRIVATE
            /MP
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
        ${FILES_MAXMOD_SOURCE_HEADLESS}
)

# WAV renderer
# ------------

add_executable(maxmod_wav_renderer)

search_source_files(tools/wav_renderer/source FILES_TOOLS_WAV_RENDERER_SOURCE)

target_sources(maxmod_wav_renderer
    PRIVATE
        ${FILES_TOOLS_WAV_RENDERER_SOURCE}
)

target_link_libraries(maxmod_wav_renderer maxmod)
