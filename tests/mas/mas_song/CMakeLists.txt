# SPDX-License-Identifier: ISC
#
# Copyright (c) 2026, Antonio Niño Díaz

# Test that we can play songs from MAS files directly.
mas_create_file_test("../../_audio_files/nb_roots.mod" "nb_roots.mas")
mas_test(mas_song "nb_roots.mas")
