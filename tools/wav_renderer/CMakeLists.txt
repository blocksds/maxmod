# SPDX-License-Identifier: ISC
#
# Copyright (c) 2026, Antonio Niño Díaz

add_executable(maxmod_wav_renderer)

search_source_files(source WAV_RENDERER_SOURCE)

target_sources(maxmod_wav_renderer
    PRIVATE
        ${WAV_RENDERER_SOURCE}
)

target_link_libraries(maxmod_wav_renderer maxmod)
