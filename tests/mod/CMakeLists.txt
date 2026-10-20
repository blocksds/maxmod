# SPDX-License-Identifier: ISC
#
# Copyright (c) 2026, Antonio Niño Díaz

# To save the reference WAV, compress it with this command:
#
#     tar -jcf output.tar.bz input.wav

# Test effect 0xx, including 000, which is a "no effect" effect. Also, test
# arpeggios when the song speed is too fast and the second note of the arpeggio
# (or the first one) can't happen.
module_test("arpeggio.mod")

# Test ECx including some corner cases (like EC0, or ECx at the same time as Bxy
# or Dxy).
module_test("cut_note.mod")

# Test EDx including some corner cases, like ED0, or EDx when the value is the
# same as the speed of the song, or just around it.
module_test("delay_note.mod")

# Test of behaviour in the following cases:
# - No instrument number, only note and effect.
# - No note, only instrument number and effect.
# - Only note, no instrument or effect
# TODO: There's a small difference in the test, but not big enough to disable it
module_test("empty_fields.mod")

# Test effect Bxy and some corner cases like having it twice in the same row.
module_test("jump_to_pattern.mod")

# Test effects Bxy and Dxy in the same row.
module_test("jump_to_pattern_plus_pattern_break.mod")

# Test E60 and E6x effects. The effect only affects the current pattern, and
# mmutil tries to detect E6x that are used before E60 because the behaviour in
# that case depends a lot on the format.
module_test("loop_effect.mod")

# Test effect EEx.
module_test("pattern_delay.mod")

# Tests effects 3xy and 5xy.
module_test("porta_to_note.mod")

# Tests effects 1xy, 2xy, E1x and E2x.
module_test("porta_up_down.mod")

# Tests effects 8xy and E8x.
module_test("panning.mod")

# Test effect Dxy and some corner cases like having it twice in the same row.
module_test("pattern_break.mod")

# Tests full range of notes from C-3 to B-8.
module_test("range_test.mod")

# Tests effect E9x, including corner cases such as x = 0, x = speed -1,
# x = speed and x = speed + 1.
module_test("retrigger_note.mod")

# Tests behaviour of effect 9xy, both for samples that loop and that don't. Also
# test jumps after the end of the loop and end of the sample.
module_test("sample_offset.mod")

# Tests behaviour of samples that loop (different parts of the sample waveform)
# and samples that don't loop. Note that this doesn't behave like S3M/XM/IT.
module_test("sample_that_loops.mod")

# Tests samples that have a default finetune value.
module_test("sample_with_finetune.mod")

# Tests effect Fxy both for values that change the speed and values that change
# the BPM. Make sure that F00 is ignored.
module_test("speed.mod")

# Tests effect 7xy and how it interacts with Cxy. Cxy should set the base volume
# for 7xy to ondulate around.
module_test("tremolo.mod")

# Test all possible valid values of effect E7x, even the invalid value E78.
module_test("tremolo_waveform.mod")

# Tests effects 4xy and 6xy.
module_test("vibrato.mod")

# Test all possible valid values of effect E4x, even the invalid value E48.
module_test("vibrato_waveform.mod")

# Test Cxy effect. Test that ECx effect sets the volume to 0 and the channel can
# be made to sound again with a Cxy effect. Also, test samples with different
# default volumes.
module_test("volume.mod")

# Tests effects Axy, EAx, EBx.
module_test("volume_slide.mod")
