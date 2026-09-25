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

/// @file maxmod.h
///
/// @brief Global include of Maxmod headless.

#ifndef MAXMOD_HEADLESS_H__
#define MAXMOD_HEADLESS_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <mm_types.h>

// ***************************************************************************
/// @defgroup headless_init Headless: Initialization/Main Functions
/// @{
// ***************************************************************************

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
///
/// @return
///     It returns true on success, false on error.
bool mmInitDefault(mm_addr soundbank, mm_word number_of_channels, mm_word sample_rate);

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
///     `total_samples * 2` because the output is stereo.
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
/// @defgroup headless_module_playback Headless: Module Playback
/// @{
// ***************************************************************************

/// Begins playback of a module.
///
/// For GBA, the module data is read directly from the cartridge space, so no
/// loading is needed.
///
/// @param module_ID
///     Index of module to be played. Values are defined in the soundbank header
///     output. (prefixed with "MOD_")
/// @param mode
///     Mode of playback. Can be MM_PLAY_LOOP (play and loop until stopped
///     manually) or MM_PLAY_ONCE (play until end).
void mmStart(mm_word module_ID, mm_pmode mode);

/// Pauses playback of the active module.
///
/// Resume with mmResume().
void mmPause(void);

/// Resume module playback.
///
/// Pause with mmPause().
void mmResume(void);

/// Stops playback of the active module.
///
/// Start again (from the beginning) with mmStart().
///
/// Any channels used by the active module will be freed.
void mmStop(void);

/// Get current number of elapsed ticks in the row being played.
///
/// @return
///     Number of elapsed ticks.
mm_word mmGetPositionTick(void);

/// Get current row being played.
///
/// @return
///     The current row.
mm_word mmGetPositionRow(void);

/// Get current pattern order being played.
///
/// @return
///     The current pattern.
mm_word mmGetPosition(void);

/// Set the current playback position.
///
/// It sets the sequence [aka order-list] position for the active module and the
/// row inside the pattern.
///
/// @param position
///     New position in module sequence.
/// @param row
///     New row in the destination pattern
void mmSetPositionEx(mm_word position, mm_word row);

/// Set the current sequence [aka order-list] position for the active module.
///
/// @param position
///     New position in module sequence.
static inline void mmSetPosition(mm_word position)
{
    mmSetPositionEx(position, 0);
}

/// Set playback position.
///
/// @deprecated
///     Alias of mmSetPosition().
///
/// @param position
///     New position in module sequence.
__attribute__((deprecated))
static inline void mmPosition(mm_word position)
{
    mmSetPositionEx(position, 0);
}

/// Used to determine if a module is playing.
///
/// @return
///     Nonzero if a module is currently playing.
mm_bool mmActive(void);

/// Use this function to change the master volume scale for module playback.
///
/// @param volume
///     New volume level. Ranges from 0 (silent) to 1024 (normal).
void mmSetModuleVolume(mm_word volume);

/// Change the master tempo for module playback.
///
/// Specifying 1024 will play the module at its normal speed. Minimum and
/// maximum values are 50% (512) and 200% (2048). Note that increasing the tempo
/// will also increase the module processing load.
///
/// It uses a fixed point (Q10) value representing tempo.
///
/// Range = 0x200 -> 0x800 = 0.5 -> 2.0
///
/// @param tempo
///     New tempo value. Tempo = (speed_percentage * 1024) / 100.
void mmSetModuleTempo(mm_word tempo);

/// Change the master pitch scale for module playback.
///
/// Specifying 1024 will play the module at its normal pitch. Minimum/Maximum
/// range of the pitch change is +-1 octave.
///
/// Range = 0x200 -> 0x800 = 0.5 -> 2.0
///
/// @param pitch
///     New pitch scale. Value = 1024 * 2^(semitones/12)
void mmSetModulePitch(mm_word pitch);

/// Play individual MAS file from RAM.
///
/// @deprecated
///     This function expects the user to pass a pointer to the MAS file
///     skipping the MAS file prefix, which isn't very intuitive. Use
///     mmPlayMAS() instead.
///
/// @warning
///     You need to initialize Maxmod with mmInit() and provide it a valid
///     soundbank even if you plan to use mmPlayModule() to play everything.
///     You can initialize mmInit() passing 0 as the number of modules and sound
///     effects.
///
/// @param address
///     Address of the MAS file, skipping the first few bytes of the prefix.
///     Add `sizeof(mm_mas_prefix)` to the pointer to your MAS file.
/// @param mode
///     Playback mode: MM_PLAY_ONCE or MM_PLAY_LOOP.
/// @param layer
///     MM_MAIN (main module layer) or MM_JINGLE (sub/jingle layer).
__attribute__((deprecated))
void mmPlayModule(uintptr_t address, mm_word mode, mm_word layer);

/// Play individual MAS file from RAM.
///
/// A soundbank is a MSL file that contains one or more MAS files. Each MAS file
/// can contain a sample or a module. This function allows you to play samples
/// or modules without the need for a soundbank.
///
/// Normally, Maxmod plays MAS files from the sound bank provided to mmInit().
/// This function lets you play MAS files outside of that soundbank.
///
/// @warning
///     You need to initialize Maxmod with a valid soundbank, or with
///     mmInitNoSoundbank() if you don't plan on using any soundbank at all.
///
/// @param address
///     Address of the MAS file.
/// @param mode
///     Playback mode: MM_PLAY_ONCE or MM_PLAY_LOOP.
/// @param layer
///     MM_MAIN (main module layer) or MM_JINGLE (sub/jingle layer).
void mmPlayMAS(uintptr_t address, mm_word mode, mm_word layer);

// ***************************************************************************
/// @}
/// @defgroup headless_jingle_playback Headless: Jingle Playback
/// @{
// ***************************************************************************

/// Plays a jingle.
///
/// Jingles are normal modules that can be mixed with the normal module
/// playback.
///
/// For GBA, the module is read directly from the cartridge space. For jingles,
/// the playback mode is fixed to MM_PLAY_ONCE.
///
/// Note that jingles must be limited to 4 channels only.
///
/// @param module_ID
///     Index of module to be played. (Defined in soundbank header)
/// @param mode
///     Mode of playback. Can be MM_PLAY_LOOP (play and loop until stopped
///     manually) or MM_PLAY_ONCE (play until end).
void mmJingleStart(mm_word module_ID, mm_pmode mode);

/// Plays a jingle.
///
/// @deprecated
///     Use mmJingleStart() instead.
///
/// @param module_ID
///     Index of module to be played. (Defined in soundbank header)
__attribute__((deprecated))
static inline void mmJingle(mm_word module_ID)
{
    mmJingleStart(module_ID, MM_PLAY_ONCE);
}

/// Pauses playback of the active jingle.
///
/// Resume with mmJingleResume().
void mmJinglePause(void);

/// Resume jingle playback.
///
/// Pause with mmJinglePause().
void mmJingleResume(void);

/// Stops playback of the active jingle.
///
/// Start again (from the beginning) with mmJingleStart().
///
/// Any channels used by the active module will be freed.
void mmJingleStop(void);

/// Check if a jingle is playing or not.
///
/// @return
///     Returns nonzero if a jingle is actively playing.
mm_bool mmJingleActive(void);

/// Check if a jingle is playing or not.
///
/// @deprecated
///     Alias of mmJingleActive().
///
/// @return
///     Returns nonzero if a jingle is actively playing.
__attribute__ ((deprecated))
static inline mm_bool mmActiveSub(void)
{
    return mmJingleActive();
}

/// Use this function to change the master volume scale for jingle playback.
///
/// @param volume
///     New volume level. Ranges from 0 (silent) to 1024 (normal).
void mmSetJingleVolume(mm_word volume);

// ***************************************************************************
/// @}
/// @defgroup headless_sound_effects Headless: Sound Effects
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
/// @param sound
///     Structure containing information about the sound to be played.
///
/// @return
///     On success, sound effect handle that can be used to modify parameters of
///     the sound effect while it is playing. On error, MM_SFXHAND_INVALID.
mm_sfxhand mmEffectEx(mm_sound_effect* sound);

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

/// Indicates if a sound effect is active or not.
///
/// @param handle
///     Sound effect handle received from mmEffect() or mmEffectEx().
///
/// @return
///     Non-zero if the sound effect is active, zero if it isn't.
mm_bool mmEffectActive(mm_sfxhand handle);

/// Stops a sound effect. The handle will be invalidated.
///
/// @note
///     It can't stop released effects because their handles are invalid.
///
/// @param handle
///     Sound effect handle received from mmEffect() or mmEffectEx().
///
/// @return
///     Non-zero if the sound was found and stopped, zero on error.
mm_word mmEffectCancel(mm_sfxhand handle);

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

#endif // MAXMOD_HEADLESS_H__
