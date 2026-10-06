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

# Test effect Jxx, including 000, which is a "no effect" effect. Also, test
# arpeggios when the song speed is too fast and the second note of the arpeggio
# (or the first one) can't happen.
s3m_test("arpeggio")

# Test SCx including some corner cases (like SC0, or SCx at the same time as Bxy
# or Cxy).
s3m_test("cut_note")

# Test SDx including some corner cases, like SD0, or SDx when the value is the
# same as the speed of the song, or just around it.
# TODO: This test is currently broken
#s3m_test("delay_note")

# Test of behaviour in the following cases:
# - No instrument number, only note and effect.
# - No note, only instrument number and effect.
# - Only note, no instrument or effect
s3m_test("empty_fields")

# It tests effects Vxx and the non-standard Wxx.
s3m_test("global_volume")

# Test initial global volume, initial tempo and speed. Change them during the
# song. Also, test the initial per-channel panning setting.
s3m_test("initial_settings")

# Test SB0 and SBx effects. The effect only affects the current pattern, but the
# destination row set by SB0 isn't preseved between patterns. If you use SBx in
# the pattern before an SB0, it will jump to the start of the pattern.
s3m_test("loop_effect")

# Test effect Bxy and some corner cases like having it twice in the same row.
s3m_test("jump_to_pattern")

# Test effects Bxy and Cxy in the same row.
s3m_test("jump_to_pattern_plus_pattern_break")

# Tests effects Gxy and Lxy.
s3m_test("porta_to_note")

# Tests effect Exx.
s3m_test("porta_up_down")

# Tests effects Xxy and S8x.
s3m_test("panning")

# It's possible to specify panning instead of volume in the volume column. This
# isn't a standard behaviour, but it's recognized by trackers. IT files support
# the same behaviour.
s3m_test("panning_in_volume_column")

# Test effects S6x and SEx.
# TODO: Fine pattern delay doesn't work correctly
#s3m_test("pattern_delay")

# Test effect Cxy and some corner cases like having it twice in the same row or
# jumping to a row number too big.
s3m_test("pattern_break")

# Tests full range of notes from C-3 to B-8.
s3m_test("range_test")

# Tests effect Qxy, including corner cases such as y = 0, y = speed -1,
# y = speed and y = speed + 1.
s3m_test("retrigger_note")

# Tests behaviour of effect Oxy, both for samples that loop and that don't.
# TODO: This test is currently broken
#s3m_test("sample_offset")

# Tests behaviour of samples that loop (different parts of the sample waveform)
# and samples that don't loop.
# TODO: This test is currently broken
#s3m_test("sample_that_loops")

# Tests samples that have a default finetune value.
s3m_test("sample_with_finetune")

# Tests effects Axx and Txx to change the speed and BPM.
s3m_test("speed")

# Tests effect Rxy and how it interacts with the volume column. The column
# should set the base volume for Rxy to ondulate around.
s3m_test("tremolo")

# Test all possible valid values of effect S4x (x goes from 0 to 3).
# TODO: This is currently broken in Maxmod, it only supports sine waves.
#s3m_test("tremolo_waveform")

# Tests effects Hxy and Kxy.
s3m_test("vibrato")

# Test all possible valid values of effect S3x (x goes from 0 to 3).
# TODO: This is currently broken in Maxmod, it only supports sine waves.
#s3m_test("vibrato_waveform")

# Test the volume column. Test that SCx effect stops the note. Setting the
# volume back up doesn't retrigger the note. Also, test samples with different
# default volumes.
# TODO: This is currently broken
#s3m_test("volume")

# Tests effect Dxy.
s3m_test("volume_slide")
