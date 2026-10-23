#!/bin/bash

# SPDX-License-Identifier: ISC
#
# Copyright (c) 2026 Antonio Niño Díaz

BLOCKSDS=${BLOCKSDS:-/opt/blocksds/core}
MMUTIL=${BLOCKSDS}/tools/mmutil/mmutil

WAV_RENDERER="$1"
INPUT_FOLDER="$2"
OUTPUT_PATH="$3"

if [ "$#" -ne 3 ]; then
    echo "Missing arguments:"
    echo ""
    echo "    $0 [wav-renderer] [input-folder-with-songs] [test-folder]"
    echo ""
    exit 1
fi

SOUNDBANK="$OUTPUT_PATH/soundbank_reference.bin"

set -e

mkdir -p "$OUTPUT_PATH"

echo "[###] Generating song reference wav files in $OUTPUT_PATH"

mod_files=`find "$INPUT_FOLDER" -type f -iname "*.mod"`
s3m_files=`find "$INPUT_FOLDER" -type f -iname "*.s3m"`
it_files=`find "$INPUT_FOLDER" -type f -iname "*.it"`
xm_files=`find "$INPUT_FOLDER" -type f -iname "*.xm"`

audio_file_paths="$mod_files $s3m_files $it_files $xm_files"

echo "[###] Generating soundbank in ${SOUNDBANK}"

$MMUTIL -y -D -o${SOUNDBANK} $audio_file_paths 1>"$OUTPUT_PATH/mmutil_reference.log" 2>&1

for audio_file_path in $audio_file_paths; do
    song_name=`basename $audio_file_path`
    wav_name="$OUTPUT_PATH/$song_name.reference.wav"

    echo "[#] Rendering $audio_file_path"

    $WAV_RENDERER "$SOUNDBANK" $song_name $wav_name
done

echo "[###] All songs rendered"
