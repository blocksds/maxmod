# SPDX-License-Identifier: ISC
#
# Copyright (c) 2026, Antonio Niño Díaz

# To save the reference WAV, compress it with this command:
#
#     tar -jcf output.tar.bz input.wav

macro(it_test test_name)
    add_test(
        NAME test_it_${test_name}

        COMMAND
            ${BASH_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/run_test.sh
                $<TARGET_FILE:maxmod_wav_renderer> ${TEST_SOUNDBANK_PATH}
                ${CMAKE_CURRENT_SOURCE_DIR} ${test_name}

        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )

    set_tests_properties(test_it_${test_name} PROPERTIES FIXTURES_REQUIRED fixture_soundbank)
endmacro()

# Test effect Jxx, including J00, which is a "continue" effect. Also, test
# arpeggios when the song speed is too fast and the second note of the arpeggio
# (or the first one) can't happen.
it_test("arpeggio")

# It tests non-standard effects Mxx and Nxx, as well as changing instrument
# volume and channel volume. It also shows how it interacts with Vxx.
# TODO: When the volume changes in a pattern with Mxx and Vxx at the same time,
# there is a small delay between both of them. Also, it looks like the initial
# change doesn't work as expected.
#it_test("channel_volume")

# Test SCx including some corner cases (like SC0, or SCx at the same time as Bxy
# or Cxy).
it_test("cut_note")

# Test SDx including some corner cases, like SD0, or SDx when the value is the
# same as the speed of the song, or just around it.
# TODO: This test is currently broken
#it_test("delay_note")

# Test of behaviour in the following cases:
# - No instrument number, only note and effect.
# - No note, only instrument number and effect.
# - Only note, no instrument or effect
# TODO: The second part of the test fails
#it_test("empty_fields")

# It tests effects Vxx and the non-standard Wxx.
it_test("global_volume")

# Test initial global volume, initial tempo and speed. Change them during the
# song. Also, test the initial per-channel panning setting.
it_test("initial_settings")

# Test SB0 and SBx effects. The effect only affects the current pattern, but the
# destination row set by SB0 isn't preseved between patterns. If you use SBx in
# the pattern before an SB0, it will jump to the start of the pattern.
# TODO: Jumps in patterns without SB0 don't seem to work as expected
#it_test("loop_effect")

# Test effect Bxy and some corner cases like having it twice in the same row.
it_test("jump_to_pattern")

# Test effects Bxy and Cxy in the same row.
it_test("jump_to_pattern_plus_pattern_break")

# Tests effects Gxy and Lxy.
it_test("porta_to_note")

# Tests effect Exx.
it_test("porta_up_down")

# Tests effects Xxy and S8x.
it_test("panning")

# It's possible to specify panning instead of volume in the volume column.
it_test("volcmd_panning")

# Test effects S6x and SEx.
# TODO: Fine pattern delay doesn't work correctly
#it_test("pattern_delay")

# Test effect Cxx and some corner cases like having it twice in the same row or
# jumping to a row number too big.
# TODO: Currently Maxmod doesn't behave correctly when Cxx jumps to a row higher
# than the size of next pattern. This needs to be fixed in Maxmod.
#it_test("pattern_break")

# Tests full range of notes from C-3 to B-8.
it_test("range_test")

# Tests effect Qxy, including corner cases such as y = 0, y = speed -1,
# y = speed and y = speed + 1.
it_test("retrigger_note")

# Tests behaviour of effect Oxy, both for samples that loop and that don't.
# TODO: This test is currently broken
#it_test("sample_offset")

# Tests behaviour of samples that loop (different parts of the sample waveform)
# and samples that don't loop.
# TODO: This test is currently broken
#it_test("sample_that_loops")

# Tests samples that have a default finetune value.
it_test("sample_with_finetune")

# Tests effects Axx and Txx to change the speed and BPM. It tests Txx for
# 'xx > 0x20', '0x1F >= xx >= 0x10' and '0x0F >= xx >= 0x00'.
it_test("speed")

# Tests effect Rxy and how it interacts with the volume column. The column
# should set the base volume for Rxy to ondulate around.
it_test("tremolo")

# Test all possible valid values of effect S4x (x goes from 0 to 3).
# TODO: This is currently broken in Maxmod, it only supports sine waves.
#it_test("tremolo_waveform")

# Tests effects Hxy, Uxy and Kxy.
it_test("vibrato")

# Test all possible valid values of effect S3x (x goes from 0 to 3).
# TODO: This is currently broken in Maxmod, it only supports sine waves.
#it_test("vibrato_waveform")

# Tests effects a0x, b0x, c0x, d0x in the volume column.
it_test("volcmd_volume_slide")

# Test the volume column. Test that SCx effect stops the note. Setting the
# volume back up doesn't retrigger the note. Also, test samples with different
# default volumes.
# TODO: This is currently broken
#it_test("volume")

# Tests effect Dxy.
it_test("volume_slide")
