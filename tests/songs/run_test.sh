#!/bin/bash

# SPDX-License-Identifier: ISC
#
# Copyright (c) 2026 Antonio Niño Díaz

if [ $# -ne 3 ]; then
    echo "Missing arguments"
    exit 1
fi

TEST_EXECUTABLE=$1
SOUNDBANK_PATH=$2
TESTS_FOLDER=$3

REFERENCE_TAR_BZ="${TESTS_FOLDER}/reference.tar.bz2"
REFERENCE_WAV="output.wav"
OUTPUT_WAV="output.test.wav"

set -e
set -x

${TEST_EXECUTABLE} ${SOUNDBANK_PATH} ${OUTPUT_WAV}

tar -xf ${REFERENCE_TAR_BZ} -C .

diff ${REFERENCE_WAV} ${OUTPUT_WAV}

exit 0
