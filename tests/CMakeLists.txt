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

# Add subdirectories with tests
# -----------------------------

# Add them after the first test so that TEST_SOUNDBANK_PATH is available there
add_subdirectory(mod)
add_subdirectory(sfx)
add_subdirectory(songs)
