# SPDX-License-Identifier: ISC
#
# Copyright (c) 2026, Antonio Niño Díaz

# To save the reference WAV, compress it with this command:
#
#     tar -jcf output.tar.bz input.wav

macro(xm_test test_name)
    add_test(
        NAME test_xm_${test_name}

        COMMAND
            ${BASH_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/run_test.sh
                $<TARGET_FILE:maxmod_wav_renderer> ${TEST_SOUNDBANK_PATH}
                ${CMAKE_CURRENT_SOURCE_DIR} ${test_name}

        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )

    set_tests_properties(test_xm_${test_name} PROPERTIES FIXTURES_REQUIRED fixture_soundbank)
endmacro()

# Test effect 0xx, including 000, which is a "no effect" effect. Also, test
# arpeggios when the song speed is too fast and the second note of the arpeggio
# (or the first one) can't happen.
xm_test("arpeggio")

# Test ECx including some corner cases (like EC0, or ECx at the same time as Bxy
# or Dxy).
xm_test("cut_note")

# Test EDx including some corner cases, like ED0, or EDx when the value is the
# same as the speed of the song, or just around it.
# TODO: This test is currently broken
#xm_test("delay_note")

# Test of behaviour in the following cases:
# - No instrument number, only note and effect.
# - No note, only instrument number and effect.
# - Only note, no instrument or effect
# TODO: The second part of the test fails
#xm_test("empty_fields")

# It tests effects Gxx and Hxx.
xm_test("global_volume")

# Test initial global volume, initial tempo and speed. Change them during the
# song. Also, test the initial per-channel panning setting.
xm_test("initial_settings")

# Test E60 and E6x effects. The effect only affects the current pattern, but the
# destination row set by E60 is preseved even in other patterns. If you use E6x
# in the pattern after E60, it will jump to the previously saved location.
# TODO: The second part of this test behaves like S3M, which is wrong.
#xm_test("loop_effect")

# Test effect Bxy and some corner cases like having it twice in the same row.
xm_test("jump_to_pattern")

# Test effects Bxy and Dxy in the same row.
xm_test("jump_to_pattern_plus_pattern_break")

# Tests effects 3xy and 5xy.
xm_test("porta_to_note")

# Tests effects 1xy, 2xy, E1x and E2x.
xm_test("porta_up_down")

# Tests effects 8xy and E8x.
xm_test("panning")

# It's possible to specify panning instead of volume in the volume column.
xm_test("volcmd_panning")

# Test effect EEx.
# TODO: Fine pattern delay doesn't work correctly
#xm_test("pattern_delay")

# Test effect Dxy and some corner cases like having it twice in the same row or
# jumping to a row number too big.
# TODO: Currently Maxmod doesn't behave correctly when Cxx jumps to a row higher
# than the size of next pattern. This needs to be fixed in Maxmod.
#xm_test("pattern_break")

# Tests full range of notes from C-3 to B-8.
xm_test("range_test")

# Tests effect Ryx, including corner cases such as x = 0, x = speed -1,
# x = speed and x = speed + 1.
# TODO: The initial note seems to be buggy
#xm_test("retrigger_note")

# Tests behaviour of effect 9xy, both for samples that loop and that don't.
# TODO: This test is currently broken
#xm_test("sample_offset")

# Tests behaviour of samples that loop (different parts of the sample waveform)
# and samples that don't loop.
# TODO: This test is currently broken
#xm_test("sample_that_loops")

# Tests samples that have a default finetune value.
# TODO: The arpeggios are buggy in OpenMPT, but maybe they are a Maxmod bug?
#xm_test("sample_with_finetune")

# Tests effect Fxy both for values that change the speed and values that change
# the BPM. F00 isn't tested, it stops the song.
xm_test("speed")

# Tests effect 7xy and how it interacts with the volume column. The volume
# column should set the base volume for 7xy to ondulate around.
xm_test("tremolo")

# Test all possible valid values of effect E7x, even the invalid value E78.
# TODO: This is currently broken in Maxmod, it only supports sine waves.
#xm_test("tremolo_waveform")

# Tests effect Txy
xm_test("tremor")

# Tests effects 4xy and 6xy.
xm_test("vibrato")

# Test all possible valid values of effect E4x, even the invalid value E48.
# TODO: This is currently broken in Maxmod, it only supports sine waves.
#xm_test("vibrato_waveform")

# Tests effects a0x, b0x, c0x, d0x in the volume column.
xm_test("volcmd_volume_slide")

# Test the volume column and effect Cxx. Test that ECx effect stops the note.
# Setting the volume back up doesn't retrigger the note. Also, test samples with
# different default volumes.
# TODO: This is currently broken
xm_test("volume")

# Tests effects Axy, EAx and EBx.
xm_test("volume_slide")
