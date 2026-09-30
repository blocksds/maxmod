#!/bin/bash

# SPDX-License-Identifier: ISC
#
# Copyright (c) 2026 Antonio Niño Díaz

rm -rf *.tar.bz

pushd build

    wav_files=`find . -name "*.wav"`

    for name in $wav_files; do

        printf "Compressing: ${name}\n"

        tar_file="${name/.wav/.tar.bz}"

        tar -jcf $tar_file $name

        mv $tar_file ../
    done

popd
