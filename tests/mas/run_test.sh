#!/bin/bash

# SPDX-License-Identifier: ISC
#
# Copyright (c) 2026 Antonio Niño Díaz

if [ $# -ne 4 ]; then
    echo "Missing arguments"
    exit 1
fi

TEST_EXECUTABLE=$1
SOUNDBANK_PATH=$2
MAS_FILE_PATH=$3
TESTS_FOLDER=$4

REFERENCE_TAR_BZ="${TESTS_FOLDER}/reference.tar.bz2"
REFERENCE_WAV="output.wav"
OUTPUT_WAV="output.test.wav"

set -e
set -x

${TEST_EXECUTABLE} ${SOUNDBANK_PATH} ${MAS_FILE_PATH} ${OUTPUT_WAV}

tar -xf ${REFERENCE_TAR_BZ} -C .

diff ${REFERENCE_WAV} ${OUTPUT_WAV}

exit 0
