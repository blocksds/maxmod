// SPDX-License-Identifier: ISC
//
// Copyright (c) 2008, Mukunda Johnson (mukunda@maxmod.org)
// Copyright (c) 2023, Lorenzooone (lollo.lollo.rbiz@gmail.com)
// Copyright (c) 2025, Antonio Niño Díaz (antonio_nd@outlook.com)

#ifndef MM_DS_COMMON_COMM_MESSAGES_H__
#define MM_DS_COMMON_COMM_MESSAGES_H__

#include <mm_types.h>

enum mm_message_ids
{
    MSG_BANK            = 0x00, // Get sound bank and number of songs and samples
    MSG_SELCHAN         = 0x01, // Select channels
    MSG_START           = 0x02, // Start module
    MSG_PAUSE           = 0x03, // Pause module
    MSG_RESUME          = 0x04, // Resume module
    MSG_STOP            = 0x05, // Stop module
    MSG_POSITION        = 0x06, // Set playback position (only the pattern, not row or tick)
    // 0x07 is reserved
    MSG_MASTERVOL       = 0x08, // Set master volume
    // 0x09 is reserved
    MSG_MASTERTEMPO     = 0x0A, // Set master tempo
    MSG_MASTERPITCH     = 0x0B, // Set master pitch
    MSG_MASTEREFFECTVOL = 0x0C, // Set master effect volume, bbaa = volume
    MSG_OPENSTREAM      = 0x0D, // Open audio stream
    MSG_CLOSESTREAM     = 0x0E, // Close audio stream
    MSG_SELECTMODE      = 0x0F, // Select audio mode

    MSG_EFFECT          = 0x10, // Play effect, default parameters
    MSG_EFFECTVOL       = 0x11, // Set effect volume
    MSG_EFFECTPAN       = 0x12, // Set effect panning
    MSG_EFFECTRATE      = 0x13, // Set effect pitch
    MSG_EFFECTMULRATE   = 0x14, // Scale effect pitch
    MSG_EFFECTOPT       = 0x15, // Set effect options
    MSG_EFFECTEX        = 0x16, // Play effect, define all parameters manually
    // 0x17 is reserved

    MSG_REVERBENABLE    = 0x18, // Enable reverb
    MSG_REVERBDISABLE   = 0x19, // Disable reverb
    MSG_REVERBCFG       = 0x1A, // Configure reverb
    MSG_REVERBSTART     = 0x1B, // Start reverb
    MSG_REVERBSTOP      = 0x1C, // Stop reverb

    MSG_EFFECTCANCELALL = 0x1D, // Stop all effects

    // 0x1E to 0x3F are reserved
};

enum mm_arm7_msg_ids
{
    MSG_ARM7_UPDATE = 0,
    MSG_ARM7_SONG_EVENT = 1,
    MSG_ARM7_STREAM_READY = 2,
    MSG_ARM7_SET_POSITION = 3,
};

#endif // MM_DS_COMMON_COMM_MESSAGES_H__
