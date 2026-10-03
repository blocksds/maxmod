# SPDX-License-Identifier: ISC
#
# Copyright (c) 2026, Antonio Niño Díaz

set(SONGS_TEST_SCRIPT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/run_test.sh)

macro(songs_test test_name)
    set(FULL_NAME test_songs_${test_name})

    add_executable(${FULL_NAME})
    target_sources(${FULL_NAME} PRIVATE main.c)
    target_link_libraries(${FULL_NAME} PRIVATE maxmod maxmod_libutils)

    # Note: CMAKE_CURRENT_SOURCE_DIR and CMAKE_CURRENT_BINARY_DIR correspond to
    # the directory that uses the macro, not the one that defines the macro.
    add_test(
        NAME ${FULL_NAME}
        COMMAND ${BASH_EXECUTABLE} ${SONGS_TEST_SCRIPT_PATH}
            $<TARGET_FILE:${FULL_NAME}> ${TEST_SOUNDBANK_PATH} ${CMAKE_CURRENT_SOURCE_DIR}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )

    set_tests_properties(${FULL_NAME} PROPERTIES FIXTURES_REQUIRED fixture_soundbank)
endmacro()

# Add the subdirectories after the macro definition
add_subdirectory(jingle_layer)
add_subdirectory(main_layer)
