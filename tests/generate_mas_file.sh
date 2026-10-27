#!/bin/bash

# SPDX-License-Identifier: ISC
#
# Copyright (c) 2026 Antonio Niño Díaz

BLOCKSDS=${BLOCKSDS:-/opt/blocksds/core}
MMUTIL=${BLOCKSDS}/tools/mmutil/mmutil

if [ $# -ne 2 ]; then
    echo "Missing arguments"
    exit 1
fi

INPUT_AUDIO_FILE="$1"
OUTPUT_MAS_FILE="$2"

set -e

echo "Generating MAS file from ${INPUT_AUDIO_FILE} in ${OUTPUT_MAS_FILE}"

$MMUTIL -y -m -o${OUTPUT_MAS_FILE} ${INPUT_AUDIO_FILE}
