# SPDX-License-Identifier: ISC
#
# Copyright (c) 2026, Antonio Niño Díaz

# To save the reference WAV, compress it with this command:
#
#     tar -jcf output.tar.bz input.wav

# Test effect Jxx, including J00, which is a "continue" effect. Also, test
# arpeggios when the song speed is too fast and the second note of the arpeggio
# (or the first one) can't happen.
module_test("arpeggio.it")

# It tests non-standard effects Mxx and Nxx, as well as changing instrument
# volume and channel volume. It also shows how it interacts with Vxx.
# TODO: When the volume changes in a pattern with Mxx and Vxx at the same time,
# there is a small delay between both of them. Also, it looks like the initial
# change doesn't work as expected.
#module_test("channel_volume.it")

# Test SCx including some corner cases (like SC0, or SCx at the same time as Bxy
# or Cxy).
module_test("cut_note.it")

# Tests effect note off/cut/fade in the note column (==/^^/~~).
# TODO: This test doesn't pass
#module_test("note_stop.it")

# Test SDx including some corner cases, like SD0, or SDx when the value is the
# same as the speed of the song, or just around it.
module_test("delay_note.it")

# Test of behaviour in the following cases:
# - No instrument number, only note and effect.
# - No note, only instrument number and effect.
# - Only note, no instrument or effect
# TODO: This test doesn't pass
#module_test("empty_fields.it")

# It tests effects Vxx and the non-standard Wxx.
module_test("global_volume.it")

# Test initial global volume, initial tempo and speed. Change them during the
# song. Also, test the initial per-channel panning setting.
module_test("initial_settings.it")

# This tests different settings for the channel panning, including surround
# (which isn't supported) and mute (which is supported).
# TODO: When a sample has a default panning, it changes the panning of the
# module channel, and that affects the following notes. This shouldn't happen,
# notes should have the default panning of the channel instead.
module_test("initial_panning.it")

# Test volume, panning, pitch and filter envelopes in instruments (filter
# envelope isn't supported!). Test envelope loop and envelope sustain (but not
# envelope continue).
module_test("instrument_envelopes.it")

# Test volume, panning, pitch and filter envelopes in instruments that start
# enabled or disabled. Then, test that they can be enabled or disabled manually
# with effects S77, S78, S79, S7A, S7B and S7C.
module_test("instrument_envelopes_on_off.it")

# Tests long and short instrument fadeouts for instruments with envelope loops
# and envelope sustain loops.
module_test("instrument_fadeout.it")

# Tests an instrument with a note map that uses multiple samples.
module_test("instrument_note_map.it")

# Test SB0 and SBx effects. The effect only affects the current pattern, and
# mmutil tries to detect SBx that are used before SB0 because the behaviour in
# that case depends a lot on the format.
module_test("loop_effect.it")

# Test effect Bxy and some corner cases like having it twice in the same row.
module_test("jump_to_pattern.it")

# Test effects Bxy and Cxy in the same row.
module_test("jump_to_pattern_plus_pattern_break.it")

# Test patterns with different numbers of rows (and with fewer channels than the
# other examples). Also, test effect Dxx jumping after the end of the following
# pattern (it should jump to the start instead).
module_test("pattern_sizes.it")

# Tests effects Gxy and Lxy.
module_test("porta_to_note.it")

# Tests effect Exx.
module_test("porta_up_down.it")

# Tests effects e0x, f0x, g0x in the volume column.
module_test("volcmd_portamento.it")

# Tests effects Xxy and S8x.
module_test("panning.it")

# It's possible to specify panning instead of volume in the volume column.
module_test("volcmd_panning.it")

# Test effects S6x and SEx.
module_test("pattern_delay.it")

# Test effect Cxx and some corner cases like having it twice in the same row or
# jumping to a row number too big.
module_test("pattern_break.it")

# Tests full range of notes from C-3 to B-8.
module_test("range_test.it")

# Tests effect Qxy, including corner cases such as y = 0, y = speed -1,
# y = speed and y = speed + 1.
module_test("retrigger_note.it")

# Tests behaviour of effect Oxy, both for samples that loop and that don't. Also
# test jumps after the end of the loop and end of the sample.
module_test("sample_offset.it")

# Tests behaviour of samples that loop (different parts of the sample waveform)
# and samples that don't loop. It also tests BIDI loops.
module_test("sample_that_loops.it")

# Tests samples that have autovibrato
module_test("sample_with_autovibrato.it")

# Tests samples that have a default finetune value.
module_test("sample_with_finetune.it")

# Tests effects Axx and Txx to change the speed and BPM. It tests Txx for
# 'xx > 0x20', '0x1F >= xx >= 0x10' and '0x0F >= xx >= 0x00'.
module_test("speed.it")

# Tests effect Rxy and how it interacts with the volume column. The column
# should set the base volume for Rxy to ondulate around.
module_test("tremolo.it")

# Test all possible valid values of effect S4x (x goes from 0 to 3).
module_test("tremolo_waveform.it")

# Tests effects Hxy, Uxy and Kxy.
module_test("vibrato.it")

# Tests volume command effect h0x.
module_test("volcmd_vibrato.it")

# Test all possible valid values of effect S3x (x goes from 0 to 3).
module_test("vibrato_waveform.it")

# Tests effects a0x, b0x, c0x, d0x in the volume column.
module_test("volcmd_volume_slide.it")

# Test the volume column. Test that SCx effect stops the note. Also, test
# samples with different default volumes.
module_test("volume.it")

# Tests effect Dxy.
module_test("volume_slide.it")

# Test different types of sample compression (and no compression at all).
module_test("sample_compression_it214.it")
module_test("sample_compression_it215.it")
module_test("sample_compression_none.it")
