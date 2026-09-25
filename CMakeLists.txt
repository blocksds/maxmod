# SPDX-License-Identifier: ISC
#
# Copyright (c) 2026, Antonio Niño Díaz

# This build system only builds the headless version, not GBA or NDS

cmake_minimum_required(VERSION 4.0)
project(maxmod)
enable_language(C)

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

# SDL3 player
# -----------

if(BUILD_SDL3_PLAYER)

    add_executable(maxmod_sdl3_player)

    search_source_files(tools/sdl3_player/source FILES_TOOLS_SDL3_PLAYER_SOURCE)

    target_sources(maxmod_sdl3_player
        PRIVATE
            ${FILES_TOOLS_SDL3_PLAYER_SOURCE}
    )

    if (EMSCRIPTEN)
        # Set output file suffix
        set_target_properties(maxmod_sdl3_player PROPERTIES SUFFIX ".html")

        # Set Emscripten linker flags
        if (CMAKE_BUILD_TYPE STREQUAL "Debug")
            target_link_options(maxmod_sdl3_player PRIVATE
                -sENVIRONMENT=web
                -sALLOW_MEMORY_GROWTH=1
                -sMALLOC=emmalloc
                -sWASM=1
                -sASSERTIONS=2
                -sVERBOSE=1
                -sEXCEPTION_DEBUG=1
                -gsource-map
                -g4

                # TODO: Make this flexible
                --embed-file ${CMAKE_SOURCE_DIR}/soundbank.bin@soundbank.bin
            )
        else()
            target_link_options(maxmod_sdl3_player PRIVATE
                -sENVIRONMENT=web
                -sALLOW_MEMORY_GROWTH=1
                -sMALLOC=emmalloc
                -sWASM=1

                # TODO: Make this flexible
                --embed-file ${CMAKE_SOURCE_DIR}/soundbank.bin@soundbank.bin
            )
        endif()

        target_link_libraries(maxmod_sdl3_player PRIVATE maxmod SDL3::SDL3-static)
    else()
        target_link_libraries(maxmod_sdl3_player PRIVATE maxmod SDL3::SDL3)
    endif()

endif()


# WAV renderer
# ------------

if (BUILD_WAV_RENDERER)

    add_executable(maxmod_wav_renderer)

    search_source_files(tools/wav_renderer/source FILES_TOOLS_WAV_RENDERER_SOURCE)

    target_sources(maxmod_wav_renderer
        PRIVATE
            ${FILES_TOOLS_WAV_RENDERER_SOURCE}
    )

    target_link_libraries(maxmod_wav_renderer maxmod)

endif()
