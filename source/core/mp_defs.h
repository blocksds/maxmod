// SPDX-License-Identifier: ISC
//
// Copyright (c) 2008, Mukunda Johnson (mukunda@maxmod.org)
// Copyright (c) 2021, Antonio Niño Díaz (antonio_nd@outlook.com)

#ifndef MP_DEFS_H
#define MP_DEFS_H

// Song 'mode' can be one of the following:
#define MPP_PLAY_LOOP   0
#define MPP_PLAY_ONCE   1
#define MPP_PLAY_JINGLE 2

#define MP_SCHANNELS    4

// Callback parameters

#define MMCB_SONGREQUEST    0x1A // nds9
#define MMCB_SAMPREQUEST    0x1B // nds9
#define MMCB_DELETESONG     0x1C // nds9
#define MMCB_DELETESAMPLE   0x1D // nds9

// #define MPCB_SAMPMEMORY     0x1E // ---
// #define MPCB_SONGMEMORY     0x1F // ---
#define MMCB_BANKMEMORY     0x1E // nds9

#define MPCB_SONGMESSAGE    0x2A // gba/nds7 song playback
#define MPCB_SONGFINISHED   0x2B // gba/nds7

#endif // MP_DEFS_H
