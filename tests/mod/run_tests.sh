#!/bin/bash

# SPDX-License-Identifier: ISC
#
# Copyright (c) 2026 Antonio Niño Díaz

BLOCKSDS=${BLOCKSDS:-/opt/blocksds/core}
MMUTIL=${BLOCKSDS}/tools/mmutil/mmutil

WAV_RENDERER=../../build/tools/wav_renderer/maxmod_wav_renderer

test_files=()

# Test effect 0xx, including 000, which is a "no effect" effect. Also, test
# arpeggios when the song speed is too fast and the second note of the arpeggio
# (or the first one) can't happen.
test_files+=("arpeggio.mod")

# Test ECx including some corner cases (like EC0, or ECx at the same time as Bxy
# or Dxy).
test_files+=("cut_note.mod")

# Test EDx including some corner cases, like ED0, or EDx when the value is the
# same as the speed of the song, or just around it.
# TODO: This test is currently broken
test_files+=("delay_note.mod")

# Test of behaviour in the following cases:
# - No instrument number, only note and effect.
# - No note, only instrument number and effect.
# - Only note, no instrument or effect
test_files+=("empty_fields.mod")

# Test effect Bxy and some corner cases like having it twice in the same row.
test_files+=("jump_to_pattern.mod")

# Test effects Bxy and Dxy in the same row.
test_files+=("jump_to_pattern_plus_pattern_break.mod")

# Tests effects 3xy and 5xy.
test_files+=("porta_to_note.mod")

# Tests effects 1xy, 2xy, E1x and E2x.
test_files+=("porta_up_down.mod")

# Tests effects 8xy and E8x.
test_files+=("panning.mod")

# Test effect Dxy and some corner cases like having it twice in the same row.
test_files+=("pattern_break.mod")

# Tests full range of notes from C-3 to B-8.
test_files+=("range_test.mod")

# Tests effect E9x, including corner cases such as x = 0, x = speed -1,
# x = speed and x = speed + 1.
test_files+=("retrigger_note.mod")

# Tests behaviour of effect 9xy, both for samples that loop and that don't.
test_files+=("sample_offset.mod")

# Tests behaviour of samples that loop (different parts of the sample waveform)
# and samples that don't loop.
# TODO: This test is currently broken
test_files+=("sample_that_loops.mod")

# Tests samples that have a default finetune value.
test_files+=("sample_with_finetune.mod")

# Tests effect Fxy both for values that change the speed and values that change
# the BPM. Make sure that F00 is ignored.
test_files+=("speed.mod")

# Tests effect 7xy and how it interacts with Cxy. Cxy should set the base volume
# for 7xy to ondulate around.
test_files+=("tremolo.mod")

# Test all possible valid values of effect E7x, even the invalid value E78.
# TODO: This is currently broken in Maxmod, it only supports sine waves.
test_files+=("tremolo_waveform.mod")

# Tests effects 4xy and 6xy.
test_files+=("vibrato.mod")

# Test all possible valid values of effect E4x, even the invalid value E48.
test_files+=("vibrato_waveform.mod")

# Test Cxy effect. Test that ECx effect sets the volume to 0 and the channel can
# be made to sound again with a Cxy effect. Also, test samples with different
# default volumes.
test_files+=("volume.mod")

# Tests effects Axy, EAx, EBx.
test_files+=("volume_slide.mod")

rm -rf build
mkdir build

mkdir build/references
for name in *.tar.bz; do
    tar -xf $name -C build/references
done

DEFAULT="\033[0m"
RED="\033[31m"
GREEN="\033[32m"
BOLD="\033[1m"

TESTS_SUCCESS=0
TESTS_TOTAL=0

FAILED_TESTS=()

$MMUTIL *.mod -obuild/soundbank.bin -D
printf "\n"

for name in ${test_files[@]}; do

    printf "${BOLD}[#####] TEST START: ${name}${DEFAULT}\n"

    wav_name="${name/.mod/.wav}"

    $WAV_RENDERER build/soundbank.bin $name build/$wav_name
    rc=$?

    if [ $rc -ne 0 ]; then
        printf "${RED}${BOLD}[#] RENDERING FAILED${DEFAULT}\n"
        FAILED_TESTS+=("$name")
    else

        diff build/$wav_name build/references/$wav_name
        rc=$?

        if [ $rc -ne 0 ]; then
            printf "${RED}${BOLD}[#] COMPARISON FAILED${DEFAULT}\n"
            FAILED_TESTS+=("$name")
        else
            TESTS_SUCCESS=$(( TESTS_SUCCESS + 1 ))
        fi
    fi

    printf "${BOLD}[#####] TEST END: ${name}${DEFAULT}\n"
    printf "\n"

    TESTS_TOTAL=$(( TESTS_TOTAL + 1 ))
done

TESTS_FAILURE=$(( TESTS_TOTAL - TESTS_SUCCESS ))

printf "${BOLD}[#] TESTS TOTAL:  ${TESTS_TOTAL}${DEFAULT}\n"
printf "${BOLD}[#] TESTS OK:     ${TESTS_SUCCESS}${DEFAULT}\n"
if [ ${TESTS_FAILURE} -ne 0 ]; then
    printf "${RED}${BOLD}[#] TESTS FAILED: ${TESTS_FAILURE}${DEFAULT}\n\n"

    for entry in ${FAILED_TESTS[@]}
    do
        echo -e "$entry"
    done
else
    printf "${BOLD}[#] TESTS FAILED: ${TESTS_FAILURE}${DEFAULT}\n"
fi

exit 0
