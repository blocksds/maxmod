# Support status of audio files

This file describes the features supported (or unsupported) of the audio formats
supported by Maxmod.

Note that FM instruments aren't supported in any file format.

## WAV: Waveform Audio File Format

Supported chunks:

- `data`: Waveform data.
- `fmt `: Only PCM format is supported. ADPCM format support may be added later.
  Other formats aren't planned
- `smpl`: Only the loop information is used, the rest is ignored.

## MOD: The ProTracker MOD format

Notes:

- Maxmod doesn't implement effect `EFx`, but it uses it as an event marker.
  Whenever `EFx` is found, it is sent to a user-defined callback.

Unsupported effects:

- `E0x` Set Filter: Configures the Amiga's LED lowpass filter.
- `E3x` Glissando Control: Configures whether tone portamento effects slide by
  semitones or not. Not widely supported.
- `E4x` Set Vibrato Waveform: Sets the waveform of future Vibrato effects.
  Maxmod only supports sine waves.
- `E5x` Set Finetune: Overrides the finetune value for the currently playing
  note.
- `E7x` Set Tremolo Waveform: Sets the waveform of future Tremolo effects.
  Maxmod only supports sine waves.
- `EFx` Invert Loop: When used with a looped sample, goes through the sample
  loop and inverts all sampling points (i.e. changes the sign) one by one at
  speed x. This effect permanently modifies the module file when encountered
  during playback.

## S3M: The Scream Tracker 3 format

Notes:

- `SFx` isn't a valid effect, but Maxmod uses it as an event marker.  Whenever
  `SFx` is found, it is sent to a user-defined callback.

Untested features:

- `Qxy` Retrigger: This needs to be tested with `Q00` at the start of a song,
  and with `x` values other than 0.

Unsupported effects:

- `Ixy` Tremor: Rapidly switches the sample volume on and off.
- `Pxy` Panning Slide or Fine Panning Slide: Slides the current channel's
  panning position left or right. Not an original Scream Tracker 3 effect.
- `S1x` Glissando Control: Configures whether tone portamento effects slide by
  semitones or not. Not widely supported.
- `S2x` Set Finetune: Overrides the current sample's C-5 frequency with a MOD
  finetune value. Considered legacy.
- `S3x` Set Vibrato Waveform: Sets the waveform of future Vibrato effects.
  Maxmod only supports sine waves.
- `S4x` Set Tremolo Waveform: Sets the waveform of future Tremolo effects
  Maxmod only supports sine waves.
- `S5x` Set Panbrello Waveform: Sets the waveform of future Panbrello effects.
  Not an original Scream Tracker 3 effect.
- `S9x` Sound Control: Executes a sound control command. Not an original Scream
  Tracker 3 effect.
- `SAx` High Offset: Sets the high offset for future Oxx commands. Not an
  original Scream Tracker 3 effect.
- `Yxy` Panbrello: Executes panbrello with speed `x` and depth `y` on the
  current note. Not an original Scream Tracker 3 effect.
- `Zxx` MIDI Macro: Executes a macro. Not an original Scream Tracker 3 effect.

## IT: The Impulse Tracker format

Notes:

- Maxmod doesn't implement effect `WIP`, but it uses it as an event marker.
  Whenever `WIP` is found, it is sent to a user-defined callback.
- `WIP` isn't a valid effect, but Maxmod uses it as an event marker.  Whenever
  `WIP` is found, it is sent to a user-defined callback.

Untested effects:

- All

## XM: The FastTracker 2 format

Notes:

- Maxmod doesn't implement effect `WIP`, but it uses it as an event marker.
  Whenever `WIP` is found, it is sent to a user-defined callback.
- `WIP` isn't a valid effect, but Maxmod uses it as an event marker.  Whenever
  `WIP` is found, it is sent to a user-defined callback.

Untested effects:

- All
