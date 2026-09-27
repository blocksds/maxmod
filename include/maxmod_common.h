// SPDX-License-Identifier: ISC
//
// Copyright (c) 2008, Mukunda Johnson (mukunda@maxmod.org)
// Copyright (c) 2025, Antonio Niño Díaz

/****************************************************************************
 *                                                          __              *
 *                ____ ___  ____ __  ______ ___  ____  ____/ /              *
 *               / __ '__ \/ __ '/ |/ / __ '__ \/ __ \/ __  /               *
 *              / / / / / / /_/ />  </ / / / / / /_/ / /_/ /                *
 *             /_/ /_/ /_/\__,_/_/|_/_/ /_/ /_/\____/\__,_/                 *
 *                                                                          *
 *                           Common Definitions                             *
 *                                                                          *
 ****************************************************************************/

/// @file maxmod_common.h
///
/// @brief Global include of Maxmod for functions common to all platforms.

#ifndef MAXMOD_COMMON_H__
#define MAXMOD_COMMON_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <mm_types.h>

// ***************************************************************************
/// @defgroup maxmod_sound_effects Sound Effects
/// @{
// ***************************************************************************

/// Plays a sound effect with default settings.
///
/// Default settings are: Volume=Max, Panning=Center, Rate=Center (specified in
/// sample).
///
/// The value returned from this function is a handle and can be used to modify
/// the sound effect while it's actively playing.
///
/// @warning
///     In DS, make sure the sample is loaded with mmLoadEffect() first.
///
/// @param sample_ID
///     Index of sample to be played. Values are defined in the soundbank
///     header. (prefixed with "SFX_")
///
/// @return
///     On success, sound effect handle that can be used to modify parameters of
///     the sound effect while it is playing. On error, MM_SFXHAND_INVALID.
mm_sfxhand mmEffect(mm_word sample_ID);

/// Plays a sound effect with custom settings.
///
/// @warning
///     In DS, make sure the sample is loaded with mmLoadEffect() first.
///
/// @param sound
///     Structure containing information about the sound to be played.
///
/// @return
///     On success, sound effect handle that can be used to modify parameters of
///     the sound effect while it is playing. On error, MM_SFXHAND_INVALID.
mm_sfxhand mmEffectEx(mm_sound_effect* sound);

#ifndef __NDS__
/// Indicates if a sound effect is active or not.
///
/// @warning
///     Function not available on NDS.
///
/// @param handle
///     Sound effect handle received from mmEffect() or mmEffectEx().
///
/// @return
///     Non-zero if the sound effect is active, zero if it isn't.
mm_bool mmEffectActive(mm_sfxhand handle);
#endif // __NDS__

/// Changes the volume of a sound effect.
///
/// @param handle
///     Sound effect handle received from mmEffect() or mmEffectEx().
/// @param volume
///     New volume level. Ranges from 0 (silent) to 255 (normal).
void mmEffectVolume(mm_sfxhand handle, mm_word volume);

/// Changes the panning of a sound effect.
///
/// @param handle
///     Sound effect handle received from mmEffect() or mmEffectEx().
/// @param panning
///     New panning level. Ranges from 0 (left) to 255 (right).
void mmEffectPanning(mm_sfxhand handle, mm_byte panning);

/// Changes the playback rate for a sound effect.
///
/// The actual playback rate depends on this value and the base frequency of the
/// sample. This parameter is a 6.10 fixed point value, passing 1024 will return
/// the sound to its original pitch, 2048 will raise the pitch by one octave,
/// and 512 will lower the pitch by an octave. To calculate a value from
/// semitones: Rate = 1024 * 2^(Semitones/12). (please don't try to do that with
/// integer maths)
///
/// @param handle
///     Sound effect handle received from mmEffect() or mmEffectEx().
/// @param rate
///     New playback rate.
void mmEffectRate(mm_sfxhand handle, mm_word rate);

/// Scales the rate of the sound effect by a certain factor.
///
/// @param handle
///     Sound effect handle received from mmEffect() or mmEffectEx().
/// @param factor
///     6.10 fixed point factor.
void mmEffectScaleRate(mm_sfxhand handle, mm_word factor);

/// Stops a sound effect. The handle will be invalidated.
///
/// @note
///     It can't stop released effects because their handles are invalid.
///
/// @warning
///     In NDS, this function returns `void`.
///
/// @param handle
///     Sound effect handle received from mmEffect() or mmEffectEx().
///
/// @return
///     Non-zero if the sound was found and stopped, zero on error.
#if defined(__NDS__) && defined(ARM9)
void mmEffectCancel(mm_sfxhand handle);
#else
mm_word mmEffectCancel(mm_sfxhand handle);
#endif

/// Marks a sound effect as unimportant.
///
/// This enables the sound effect to be interrupted by music/other sound effects
/// if the need arises. The handle will be invalidated.
///
/// @warning
///     A released effect can't be stopped with mmEffectCancel() because the
///     handle is invalid. It can be cancelled with mmEffectCancelAll().
///
/// @param handle
///     Sound effect handle received from mmEffect() or mmEffectEx().
void mmEffectRelease(mm_sfxhand handle);

/// Set master volume scale for effect playback.
///
/// @param volume
///     Master volume. 0->1024 representing 0%->100% volume
void mmSetEffectsVolume(mm_word volume);

/// Stop all sound effects and reset the effect system.
///
/// It stops even released effects.
void mmEffectCancelAll(void);

// ***************************************************************************
/// @}
// ***************************************************************************

#ifdef __cplusplus
}
#endif

#endif // MAXMOD_COMMON_H__
