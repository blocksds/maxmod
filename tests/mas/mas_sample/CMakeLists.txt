# SPDX-License-Identifier: ISC
#
# Copyright (c) 2026, Antonio Niño Díaz

# Test that we can play MAS files as sound effects.
mas_create_file_test("../../_audio_files/helicopter.wav" "helicopter.mas")
mas_test(mas_sample "helicopter.mas")
