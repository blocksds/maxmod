# SPDX-License-Identifier: ISC
#
# Copyright (c) 2026, Antonio Niño Díaz

# To save the reference WAV, compress it with this command:
#
#     tar -jcf output.tar.bz input.wav

# Test effect 0xx, including 000, which is a "no effect" effect. Also, test
# arpeggios when the song speed is too fast and the second note of the arpeggio
# (or the first one) can't happen.
module_test("arpeggio.xm")

# Test ECx including some corner cases (like EC0, or ECx at the same time as Bxy
# or Dxy).
module_test("cut_note.xm")

# Tests effect Kxx and note off in the note column (==).
# TODO: This test doesn't pass
#module_test("note_stop.xm")

# Test EDx including some corner cases, like ED0, or EDx when the value is the
# same as the speed of the song, or just around it.
module_test("delay_note.xm")

# Test of behaviour in the following cases:
# - No instrument number, only note and effect.
# - No note, only instrument number and effect.
# - Only note, no instrument or effect
# TODO: There's a small difference in the test, but not big enough to disable it
module_test("empty_fields.xm")

# It tests effects Gxx and Hxx.
module_test("global_volume.xm")

# Test initial global volume, initial tempo and speed. Change them during the
# song. Also, test the initial per-channel panning setting.
module_test("initial_settings.xm")

# Test volume and panning envelopes in instruments. Test envelope loop and
# envelope sustain.
module_test("instrument_envelopes.xm")

# Tests long and short instrument fadeouts for instruments with envelope loops
# and envelope sustain loops.
module_test("instrument_fadeout.xm")

# Tests an instrument with a note map that uses multiple samples.
module_test("instrument_note_map.xm")

# Test what happens when there's a stereo sample.
module_test("sample_stereo.xm")

# Test E60 and E6x effects. The effect only affects the current pattern, and
# mmutil tries to detect E6x that are used before E60 because the behaviour in
# that case depends a lot on the format.
module_test("loop_effect.xm")

# Test effect Bxy and some corner cases like having it twice in the same row.
module_test("jump_to_pattern.xm")

# Test effects Bxy and Dxy in the same row.
module_test("jump_to_pattern_plus_pattern_break.xm")

# Test patterns with different numbers of rows (and with fewer channels than the
# other examples). Also, test effect Dxx jumping after the end of the following
# pattern (it should jump to the start instead).
module_test("pattern_sizes.xm")

# Tests effects 3xy and 5xy.
module_test("porta_to_note.xm")

# Tests effects 1xy, 2xy, E1x, E2x, X1x and X2x.
module_test("porta_up_down.xm")

# Tests effect g0x in the volume column.
module_test("volcmd_portamento.xm")

# Tests effects 8xy and E8x.
module_test("panning.xm")

# It's possible to specify panning instead of volume in the volume column.
# However, they were buggy in FT2, so XM players need to emulate the bugs.
module_test("volcmd_panning.xm")

# Test effects EEx and X6x.
module_test("pattern_delay.xm")

# Test effect Dxy and some corner cases like having it twice in the same row or
# jumping to a row number too big.
module_test("pattern_break.xm")

# Tests full range of notes from C-3 to B-8.
module_test("range_test.xm")

# Tests effect Ryx and E9x, including corner cases such as x = 0, x = speed -1,
# x = speed and x = speed + 1. Note that Ryx is very buggy, according to the
# OpenMPT wiki, so we shouldn't focus too much on it.
module_test("retrigger_note.xm")

# Tests behaviour of effect 9xy, both for samples that loop and that don't. Also
# test jumps after the end of the loop and end of the sample.
module_test("sample_offset.xm")

# Tests behaviour of samples that loop (different parts of the sample waveform)
# and samples that don't loop. It also tests BIDI loops.
module_test("sample_that_loops.xm")

# Tests samples that have autovibrato
module_test("sample_with_autovibrato.xm")

# Tests samples that have a default finetune value.
# TODO: The arpeggios are buggy in OpenMPT, but maybe they are a Maxmod bug?
#module_test("sample_with_finetune.xm")

# Tests effect Fxy both for values that change the speed and values that change
# the BPM. F00 isn't tested, it stops the song.
module_test("speed.xm")

# Tests effect 7xy and how it interacts with the volume column. The volume
# column should set the base volume for 7xy to ondulate around.
module_test("tremolo.xm")

# Test all possible valid values of effect E7x, even the invalid value E78.
module_test("tremolo_waveform.xm")

# Tests effect Txy
module_test("tremor.xm")

# Tests effects 4xy and 6xy.
module_test("vibrato.xm")

# Tests volume command effects h0x and uxx.
module_test("volcmd_vibrato.xm")

# Test all possible valid values of effect E4x, even the invalid value E48.
module_test("vibrato_waveform.xm")

# Tests effects a0x, b0x, c0x, d0x in the volume column.
module_test("volcmd_volume_slide.xm")

# Test the volume column and effect Cxx. Also, test samples with different
# default volumes.
module_test("volume.xm")

# Tests effects Axy, EAx and EBx.
module_test("volume_slide.xm")
