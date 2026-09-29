// SPDX-License-Identifier: ISC
//
// Copyright (c) 2008, Mukunda Johnson (mukunda@maxmod.org)
// Copyright (c) 2025-2026, Antonio Niño Díaz

/****************************************************************************
 *                                                          __              *
 *                ____ ___  ____ __  ______ ___  ____  ____/ /              *
 *               / __ '__ \/ __ '/ |/ / __ '__ \/ __ \/ __  /               *
 *              / / / / / / /_/ />  </ / / / / / /_/ / /_/ /                *
 *             /_/ /_/ /_/\__,_/_/|_/_/ /_/ /_/\____/\__,_/                 *
 *                                                                          *
 *                        Headless Definitions                              *
 *                                                                          *
 ****************************************************************************/

/// @file maxmod_headless.h
///
/// @brief Global include of Maxmod headless.

#ifndef MAXMOD_HEADLESS_H__
#define MAXMOD_HEADLESS_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <mm_types.h>
#include <maxmod_common.h>

// ***************************************************************************
/// @defgroup headless_init Headless: Initialization/Main Functions
/// @{
// ***************************************************************************

/// Output formats supported by Maxmod.
typedef enum {
    MM_OUTFMT_STEREO_U8 = 0, ///< Unsigned 8-bit stereo output
    MM_OUTFMT_STEREO_S8 = 1, ///< Signed 8-bit stereo output
} mm_headless_output_format;

/// Initialize Maxmod with default settings.
///
/// @param soundbank
///     Memory address of soundbank (in ROM). A soundbank file can be created
///     with the Maxmod Utility.
/// @param number_of_channels
///     Number of module/mixing channels to allocate. Must be greater or equal
///     to the channel count in your modules. The maximum value allowed is 256.
/// @param sample_rate
///     The sample rate to be used by Maxmod. Any value is allowed.
/// @param output_mode
///     Format of the output of Maxmod.
///
/// @return
///     It returns true on success, false on error.
bool mmInitDefault(mm_addr soundbank, mm_word number_of_channels,
                   mm_word sample_rate, mm_headless_output_format output_format);

/// Deinitializes Maxmod.
///
/// If Maxmod was initialized with mmInitDefault(), it also frees the memory
/// allocated by it.
///
/// If Maxmod was initialized with mmInit(), the user is responsible for freeing
/// the memory passed to it when Maxmod was initialized.
///
/// @return
///     It returns true on success, false on error.
bool mmEnd(void);

/// Install handler to receive song events.
///
/// Use this function to receive song events. Song events occur in two
/// situations. One is by special pattern data in a module (which is triggered
/// by SFx/EFx commands). The other occurs when a module finishes playback (in
/// MM_PLAY_ONCE mode).
///
/// During the song event, Maxmod is in the middle of module processing. Avoid
/// using any Maxmod related functions during your song event handler since they
/// may cause problems in this situation.
///
/// Check the song events tutorial in the documentation for more information.
///
/// @param handler
///     Function pointer to event handler.
void mmSetEventHandler(mm_callback handler);

/// Returns the event handler previously installed by the user.
///
/// @return
///     Function pointer to the event handler currently installed.
mm_callback mmGetEventHandler(void);

/// This is the main routine-function that processes music and updates the sound
/// output.
///
/// This function must be called whenever the user wants Maxmod to generate new
/// samples and write them to a buffer.
///
/// The output format is currently hardcoded to 8-bit signed samples. The output
/// is stereo, and left and right channels are interleaved (L, R, L, R ...).
///
/// @param buffer
///     Destination buffer. The size must be `total_samples * 2`.
/// @param total_samples
///     The number of samples to save to the buffer. In total, it saves
///     `total_samples * 2` because the output is 8-bit stereo.
void mmFrame(mm_addr buffer, mm_word total_samples);

/// Returns the number of modules available in the soundbank.
///
/// @return
///     The number of modules.
mm_word mmGetModuleCount(void);

/// Returns the number of samples available in the soundbank.
///
/// @note
///     This number includes the samples used by all the songs in the soundbank,
///     not just the sound effects in WAV format.
///
/// @return
///     The number of samples.
mm_word mmGetSampleCount(void);

// ***************************************************************************
/// @}
// ***************************************************************************

#ifdef __cplusplus
}
#endif

#endif // MAXMOD_HEADLESS_H__
