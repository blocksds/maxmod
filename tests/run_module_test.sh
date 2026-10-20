#!/bin/bash

# SPDX-License-Identifier: ISC
#
# Copyright (c) 2026 Antonio Niño Díaz

if [ $# -ne 5 ]; then
    echo "Missing arguments"
    exit 1
fi

WAV_RENDERER=$1
WAV_DIFF=$2
SOUNDBANK_PATH=$3
TESTS_FOLDER=$4
MOD_FILE=$5

# Get name without extension
TEST_NAME="${MOD_FILE%.*}"

REFERENCE_TAR_BZ="${TESTS_FOLDER}/${TEST_NAME}.tar.bz2"
REFERENCE_WAV="${TEST_NAME}.wav"
OUTPUT_WAV="${TEST_NAME}.test.wav"

set -e
set -x

${WAV_RENDERER} ${SOUNDBANK_PATH} ${MOD_FILE} ${OUTPUT_WAV}

tar -xf ${REFERENCE_TAR_BZ} -C .

${WAV_DIFF} ${REFERENCE_WAV} ${OUTPUT_WAV}

exit 0
