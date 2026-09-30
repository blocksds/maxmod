# SPDX-License-Identifier: ISC
#
# Copyright (c) 2026, Antonio Niño Díaz

if(BUILD_SDL3_PLAYER)
    add_subdirectory(sdl3_player)
endif()

if (BUILD_WAV_RENDERER)
    add_subdirectory(wav_renderer)
endif()
