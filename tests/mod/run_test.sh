#!/bin/bash

# SPDX-License-Identifier: ISC
#
# Copyright (c) 2026 Antonio Niño Díaz

if [ $# -ne 4 ]; then
    echo "Missing arguments"
    exit 1
fi

WAV_RENDERER=$1
SOUNDBANK_PATH=$2
TESTS_FOLDER=$3
TEST_NAME=$4

REFERENCE_TAR_BZ="${TESTS_FOLDER}/${TEST_NAME}.tar.bz"
REFERENCE_WAV="${TEST_NAME}.wav"
OUTPUT_WAV="${TEST_NAME}.test.wav"
MOD_FILE="${TEST_NAME}.mod"

echo "Extracting reference WAV files..."

set -e
set -x

tar -xf ${REFERENCE_TAR_BZ} -C .

${WAV_RENDERER} ${SOUNDBANK_PATH} ${MOD_FILE} ${OUTPUT_WAV}

diff ${REFERENCE_WAV} ${OUTPUT_WAV}

exit 0
