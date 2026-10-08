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

REFERENCE_TAR_BZ="${TESTS_FOLDER}/${TEST_NAME}.tar.bz2"
REFERENCE_WAV="${TEST_NAME}.wav"
OUTPUT_WAV="${TEST_NAME}.test.wav"
S3M_FILE="${TEST_NAME}.xm"

set -e
set -x

${WAV_RENDERER} ${SOUNDBANK_PATH} ${S3M_FILE} ${OUTPUT_WAV}

tar -xf ${REFERENCE_TAR_BZ} -C .

diff ${REFERENCE_WAV} ${OUTPUT_WAV}

exit 0
