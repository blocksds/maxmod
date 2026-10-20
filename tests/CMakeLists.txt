# SPDX-License-Identifier: ISC
#
# Copyright (c) 2026, Antonio Niño Díaz

# Test that creates the soundbank be used by all other tests
# ----------------------------------------------------------

set(TEST_SOUNDBANK_PATH ${CMAKE_CURRENT_BINARY_DIR}/soundbank.bin)

find_program(BASH_EXECUTABLE NAMES bash REQUIRED)

add_test(
    NAME soundbank_create
    COMMAND ${BASH_EXECUTABLE} generate_soundbank.sh ${TEST_SOUNDBANK_PATH}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

# This test creates the fixture soundbank that all other tests depend on.
set_tests_properties(soundbank_create PROPERTIES FIXTURES_SETUP fixture_soundbank)

# Macro to generate tests for module files
# ----------------------------------------

set(TEST_RUNNER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/run_module_test.sh)

macro(module_test test_name)

    get_filename_component(name_base ${test_name} NAME_WE)
    get_filename_component(name_ext_with_dot ${test_name} EXT)
    string(REPLACE "." "" name_ext ${name_ext_with_dot})

    set(cmake_test_name "test_${name_ext}_${name_base}")

    add_test(
        NAME ${cmake_test_name}

        COMMAND
            ${BASH_EXECUTABLE} ${TEST_RUNNER_SCRIPT}
                $<TARGET_FILE:maxmod_wav_renderer>
                $<TARGET_FILE:maxmod_wav_diff>
                ${TEST_SOUNDBANK_PATH}
                ${CMAKE_CURRENT_SOURCE_DIR}
                ${test_name}

        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )

    set_tests_properties(${cmake_test_name} PROPERTIES FIXTURES_REQUIRED fixture_soundbank)
endmacro()

# Add subdirectories with tests
# -----------------------------

# Add them after the first test so that TEST_SOUNDBANK_PATH is available there
add_subdirectory(it)
add_subdirectory(mod)
add_subdirectory(s3m)
add_subdirectory(sfx)
add_subdirectory(songs)
add_subdirectory(xm)
