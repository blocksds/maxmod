# SPDX-License-Identifier: ISC
#
# Copyright (c) 2026, Antonio Niño Díaz

# To save the reference WAV, compress it with this command:
#
#     tar -jcf output.tar.bz input.wav

macro(s3m_test test_name)
    add_test(
        NAME test_s3m_${test_name}

        COMMAND
            ${BASH_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/run_test.sh
                $<TARGET_FILE:maxmod_wav_renderer> ${TEST_SOUNDBANK_PATH}
                ${CMAKE_CURRENT_SOURCE_DIR} ${test_name}

        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )

    set_tests_properties(test_s3m_${test_name} PROPERTIES FIXTURES_REQUIRED fixture_soundbank)
endmacro()

# Test effect 0xx, including 000, which is a "no effect" effect. Also, test
# arpeggios when the song speed is too fast and the second note of the arpeggio
# (or the first one) can't happen.
s3m_test("arpeggio")

# Test ECx including some corner cases (like EC0, or ECx at the same time as Bxy
# or Dxy).
s3m_test("cut_note")

# Test EDx including some corner cases, like ED0, or EDx when the value is the
# same as the speed of the song, or just around it.
# TODO: This test is currently broken
#s3m_test("delay_note")

# Test of behaviour in the following cases:
# - No instrument number, only note and effect.
# - No note, only instrument number and effect.
# - Only note, no instrument or effect
s3m_test("empty_fields")

# Test effect Bxy and some corner cases like having it twice in the same row.
s3m_test("jump_to_pattern")

# Test effects Bxy and Dxy in the same row.
s3m_test("jump_to_pattern_plus_pattern_break")

# Tests effects 3xy and 5xy.
s3m_test("porta_to_note")

# Tests effects 1xy, 2xy, E1x and E2x.
s3m_test("porta_up_down")

# Tests effects 8xy and E8x.
s3m_test("panning")

# Test effect Dxy and some corner cases like having it twice in the same row.
s3m_test("pattern_break")

# Tests full range of notes from C-3 to B-8.
s3m_test("range_test")

# Tests effect E9x, including corner cases such as x = 0, x = speed -1,
# x = speed and x = speed + 1.
s3m_test("retrigger_note")

# Tests behaviour of effect 9xy, both for samples that loop and that don't.
# TODO: This test is currently broken
#s3m_test("sample_offset")

# Tests behaviour of samples that loop (different parts of the sample waveform)
# and samples that don't loop.
# TODO: This test is currently broken
#s3m_test("sample_that_loops")

# Tests samples that have a default finetune value.
s3m_test("sample_with_finetune")

# Tests effect Fxy both for values that change the speed and values that change
# the BPM. Make sure that F00 is ignored.
s3m_test("speed")

# Tests effect 7xy and how it interacts with Cxy. Cxy should set the base volume
# for 7xy to ondulate around.
s3m_test("tremolo")

# Test all possible valid values of effect E7x, even the invalid value E78.
# TODO: This is currently broken in Maxmod, it only supports sine waves.
#s3m_test("tremolo_waveform")

# Tests effects 4xy and 6xy.
s3m_test("vibrato")

# Test all possible valid values of effect E4x, even the invalid value E48.
# TODO: This is currently broken in Maxmod, it only supports sine waves.
#s3m_test("vibrato_waveform")

# Test Cxy effect. Test that ECx effect sets the volume to 0 and the channel can
# be made to sound again with a Cxy effect. Also, test samples with different
# default volumes.
# TODO: This is currently broken
#s3m_test("volume")

# Tests effects Axy, EAx, EBx.
s3m_test("volume_slide")
