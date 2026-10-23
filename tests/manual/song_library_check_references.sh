#!/bin/bash

# SPDX-License-Identifier: ISC
#
# Copyright (c) 2026 Antonio Niño Díaz

BLOCKSDS=${BLOCKSDS:-/opt/blocksds/core}
MMUTIL=${BLOCKSDS}/tools/mmutil/mmutil

DEFAULT="\033[0m"
RED="\033[31m"

WAV_RENDERER="$1"
WAV_DIFF="$2"
INPUT_FOLDER="$3"
OUTPUT_PATH="$4"

if [ "$#" -ne 4 ]; then
    echo "Missing arguments:"
    echo ""
    echo "    $0 [wav-renderer] [wav-diff] [input-folder-with-songs] [test-folder]"
    echo ""
    exit 1
fi

SOUNDBANK="$OUTPUT_PATH/soundbank_test.bin"

set -e

mkdir -p "$OUTPUT_PATH"

echo "[###] Generating song reference wav files in $OUTPUT_PATH"

mod_files=`find "$INPUT_FOLDER" -type f -iname "*.mod"`
s3m_files=`find "$INPUT_FOLDER" -type f -iname "*.s3m"`
it_files=`find "$INPUT_FOLDER" -type f -iname "*.it"`
xm_files=`find "$INPUT_FOLDER" -type f -iname "*.xm"`

audio_file_paths="$mod_files $s3m_files $it_files $xm_files"

echo "[###] Generating soundbank in ${SOUNDBANK}"

$MMUTIL -y -D -o${SOUNDBANK} $audio_file_paths 1>"$OUTPUT_PATH/mmutil_test.log" 2>&1

set +e

TESTS_SUCCESS=0
TESTS_TOTAL=0

FAILED_TESTS=()

for audio_file_path in $audio_file_paths; do
    song_name=`basename $audio_file_path`
    test_wav_name="$OUTPUT_PATH/$song_name.test.wav"
    reference_wav_name="$OUTPUT_PATH/$song_name.reference.wav"

    echo "[#] Rendering $audio_file_path"

    $WAV_RENDERER "$SOUNDBANK" $song_name $test_wav_name

    $WAV_DIFF $reference_wav_name $test_wav_name
    if [ "$?" -ne 0 ]; then
        printf "${RED}error: Comparison failed!${DEFAULT}\n"
        FAILED_TESTS+=("$audio_file_path")
    else
        TESTS_SUCCESS=$(( TESTS_SUCCESS + 1 ))
    fi

    TESTS_TOTAL=$(( TESTS_TOTAL + 1 ))
done

printf "[###] All songs compared\n"
printf "\n"

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

