# SPDX-License-Identifier: ISC
#
# Copyright (c) 2026, Antonio Niño Díaz

set(BASE_NAME basic)
set(TEST_NAME test_sfx_${BASE_NAME})

add_executable(${TEST_NAME})
target_sources(${TEST_NAME} PRIVATE main.c)
target_link_libraries(${TEST_NAME} PRIVATE maxmod maxmod_libutils)

add_test(
    NAME ${TEST_NAME}
    COMMAND $<TARGET_FILE:${TEST_NAME}> ${TEST_SOUNDBANK_PATH}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

# TODO: Compare reference with the output

set_tests_properties(${TEST_NAME} PROPERTIES FIXTURES_REQUIRED fixture_soundbank)
