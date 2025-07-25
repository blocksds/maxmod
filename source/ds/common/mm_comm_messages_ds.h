// SPDX-License-Identifier: ISC
//
// Copyright (c) 2008, Mukunda Johnson (mukunda@maxmod.org)
// Copyright (c) 2023, Lorenzooone (lollo.lollo.rbiz@gmail.com)
// Copyright (c) 2025, Antonio Niño Díaz (antonio_nd@outlook.com)

#ifndef MM_COMM_MESSAGES_DS_H
#define MM_COMM_MESSAGES_DS_H

#include <mm_types.h>

enum message_ids {
    MSG_BANK = 0x00,
    MSG_SELCHAN,
    MSG_START,
    MSG_PAUSE,
    MSG_RESUME,
    MSG_STOP,
    MSG_POSITION,
    MSG_STARTSUB,
    MSG_MASTERVOL,
    MSG_MASTERVOLSUB,
    MSG_MASTERTEMPO,
    MSG_MASTERPITCH,
    MSG_MASTEREFFECTVOL,
    MSG_OPENSTREAM,
    MSG_CLOSESTREAM,
    MSG_SELECTMODE,
    MSG_EFFECT,
    MSG_EFFECTVOL,
    MSG_EFFECTPAN,
    MSG_EFFECTRATE,
    MSG_EFFECTMULRATE,
    MSG_EFFECTOPT,
    MSG_EFFECTEX,
    MSG_UNUSED,
    MSG_REVERBENABLE,
    MSG_REVERBDISABLE,
    MSG_REVERBCFG,
    MSG_REVERBSTART,
    MSG_REVERBSTOP,
    MSG_EFFECTCANCELALL
};

enum mm_arm7_msg_ids {
    MSG_ARM7_UPDATE = 0,
    MSG_ARM7_SONG_EVENT = 1,
    MSG_ARM7_STREAM_READY = 2,
};

#endif // MM_COMM_MESSAGES_DS_H
