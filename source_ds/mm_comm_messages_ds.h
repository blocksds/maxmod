// SPDX-License-Identifier: ISC
//
// Copyright (c) 2008, Mukunda Johnson (mukunda@maxmod.org)
// Copyright (c) 2023, Lorenzooone (lollo.lollo.rbiz@gmail.com)

#ifndef MM_COMM_MESSAGES_DS_H
#define MM_COMM_MESSAGES_DS_H

#include "mm_types.h"

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

#endif // MM_MAIN_DS_H
