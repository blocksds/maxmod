# SPDX-License-Identifier: ISC
#
# Copyright (c) 2026, Antonio Niño Díaz

add_library(maxmod_libutils)

search_source_files(include LIBUTILS_INCLUDE)
search_source_files(source LIBUTILS_SOURCE)

target_sources(maxmod_libutils
    PUBLIC
        ${LIBUTILS_INCLUDE}
    PRIVATE
        ${LIBUTILS_SOURCE}
)

target_include_directories(maxmod_libutils
    PUBLIC
        include
    PRIVATE
        source
)

target_sources(maxmod_libutils
    PRIVATE
        ${LIBUTILS_SOURCE}
)
