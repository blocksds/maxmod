# SPDX-License-Identifier: ISC
#
# Copyright (c) 2026, Antonio Niño Díaz

add_executable(maxmod_wav_diff)

search_source_files(source WAV_DIFF_SOURCE)

target_sources(maxmod_wav_diff
    PRIVATE
        ${WAV_DIFF_SOURCE}
)

target_link_libraries(maxmod_wav_diff PRIVATE maxmod_libutils)
