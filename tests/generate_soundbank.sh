#!/bin/bash

# SPDX-License-Identifier: ISC
#
# Copyright (c) 2026 Antonio Niño Díaz

BLOCKSDS=${BLOCKSDS:-/opt/blocksds/core}
MMUTIL=${BLOCKSDS}/tools/mmutil/mmutil

output_path="soundbank.bin"

if [ ! -z "$1" ]; then
    output_path="$1"
fi

set -e

echo "Generating soundbank in ${output_path}"

mod_files=`find . -type f -iname "*.mod"`
s3m_files=`find . -type f -iname "*.s3m"`
it_files=`find . -type f -iname "*.it"`
xm_files=`find . -type f -iname "*.xm"`
wav_files=`find . -type f -iname "*.wav"`

audio_files="$mod_files $s3m_files $it_files $xm_files $wav_files"

$MMUTIL -D -o${output_path} $audio_files
