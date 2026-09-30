# SPDX-License-Identifier: ISC
#
# Copyright (c) 2026, Antonio Niño Díaz

add_executable(maxmod_sdl3_player)

search_source_files(source SDL3_PLAYER_SOURCE)

target_sources(maxmod_sdl3_player
    PRIVATE
    ${SDL3_PLAYER_SOURCE}
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
        )
    else()
        target_link_options(maxmod_sdl3_player PRIVATE
            -sENVIRONMENT=web
            -sALLOW_MEMORY_GROWTH=1
            -sMALLOC=emmalloc
            -sWASM=1
        )
    endif()

    # The SHELL syntax is needed to allow for duplicated arguments
    target_link_options(maxmod_sdl3_player PRIVATE
        # TODO: Make this flexible for the soundbank
        "SHELL:--embed-file ${CMAKE_SOURCE_DIR}/soundbank.bin@soundbank.bin"
        "SHELL:--embed-file ${CMAKE_SOURCE_DIR}/tools/sdl3_player/graphics/default_font.png@default_font.png"
    )

    target_link_libraries(maxmod_sdl3_player PRIVATE SDL3::SDL3-static)
else()
    target_link_libraries(maxmod_sdl3_player PRIVATE SDL3::SDL3)
endif()

target_link_libraries(maxmod_sdl3_player PRIVATE maxmod maxmod_libutils)
