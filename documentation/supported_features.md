# Support status of audio files

This file describes the features supported (or unsupported) of the audio formats
supported by Maxmod.

Note that FM instruments aren't supported in S3M songs.

The information in this page has been obtained from the
[OpenMPT wiki](https://wiki.openmpt.org/Manual:_Effect_Reference).

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

- `SFx` isn't a valid effect, but Maxmod uses it as an event marker. Whenever
  `SFx` is found, it is sent to a user-defined callback.

Untested features:

- `Qxy` Retrigger: This needs to be tested with `Q00` at the start of a song,
  and with `x` values other than 0.
- `^^`: Note cut in note column.

Unsupported effects:

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
- `SAx` High Offset: Sets the high offset for future `Oxx` commands. Not an
  original Scream Tracker 3 effect.
- `Yxy` Panbrello: Executes panbrello with speed `x` and depth `y` on the
  current note. Not an original Scream Tracker 3 effect.
- `Zxx` MIDI Macro: Executes a macro. Not an original Scream Tracker 3 effect.

## IT: The Impulse Tracker format

Notes:

- `SFx` is a valid effect (Set Active Macro), but it is barely used and Maxmod
  uses it as an event marker instead. Whenever `SFx` is found, it is sent to a
  user-defined callback.

Untested features:

- `S73` NNA Note Cut: Sets the currently active note's New Note Action to Note Cut.
- `S74` NNA Note Continue: Sets the currently active note's New Note Action to Continue.
- `S75` NNA Note Off: Sets the currently active note's New Note Action to Note Off.
- `S76` NNA Note Fade: Sets the currently active note's New Note Action to Note Fade.
- `S77` Volume Envelope Off: Disables the currently active note's volume envelope.
- `S78` Volume Envelope On: Enables the currently active note's volume envelope.
- `^^`: Note cut in note column.
- `==`: Note off in note column.
- `~~`: Note fade in note column.

Untested volume column effects:

- `h0x` Vibrato Depth: Executes a vibrato with depth `x` and speed from the last
  `Hxy` or `Uxy` command.
- Better tests with patterns with different sizes.

Unsupported effects:

- `Ixy` Tremor: Rapidly switches the sample volume on and off.
- `Pxy` Panning Slide or Fine Panning Slide: Slides the current channel's
  panning position left or right.
- `S1x` Glissando Control: Configures whether tone portamento effects slide by
  semitones or not. Not widely supported.
- `S2x` Set Finetune: Overrides the current sample's C-5 frequency with a MOD
  finetune value. Considered legacy.
- `S3x` Set Vibrato Waveform: Sets the waveform of future Vibrato effects.
  Maxmod only supports sine waves.
- `S4x` Set Tremolo Waveform: Sets the waveform of future Tremolo effects
  Maxmod only supports sine waves.
- `S5x` Set Panbrello Waveform: Sets the waveform of future Panbrello effects.
- `S70` Past Note Cut: Cuts all notes playing as a result of New Note Actions on
  the current channel.
- `S71` Past Note Off: Sends a Note Off to all notes playing as a result of New
  Note Actions on the current channel.
- `S72` Past Note Fade: Fades out all notes playing as a result of New Note
  Actions on the current channel.
- `S79` Panning Envelope Off: Disables the currently active note's panning
  envelope.
- `S7A` Panning Envelope On: Enables the currently active note's panning
  envelope.
- `S7B` Pitch Envelope Off: Disables the currently active note's pitch or filter
  envelope.
- `S7C` Pitch Envelope On: Enables the currently active note's pitch envelope.
- `S9x` Sound Control: Executes a sound control command.
- `SAx` High Offset: Sets the high offset for future `Oxx` commands.
- `SFx` Set Active Macro: Sets the current channel's active parametered macro.
- `Yxy` Panbrello: Executes panbrello with speed `x` and depth `y` on the
  current note.
- `Zxx` MIDI Macro: Executes a macro.
- `\xx` Smooth MIDI Macro: Executes an interpolated MIDI Macro. This effect is a
  ModPlug hack.

## XM: The FastTracker 2 format

Notes:

- `EFx` is a valid effect (Set Active Macro), but it is barely used and Maxmod
  uses it as an event marker instead. Whenever `EFx` is found, it is sent to a
  user-defined callback.

Untested features:

- `E9x` Retrigger: Retriggers the current note every `x` ticks.
- `Kxx` Key Off: Triggers a Note Off command after `xx` ticks.
- `Lxx` Set Envelope Position: Sets the volume envelope playback position to
  `xx` ticks.
- `X1x` Extra Fine Portamento Up: Similar to `E1x`, but with 4 times the
  precision.
- `X2x` Extra Fine Portamento Down: Similar to `E2x`, but with 4 times the
  precision.
- `==`: Note off in note column.
- Better tests with patterns with different sizes.

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
- `EFx` Set Active Macro: Selects the active parametered macro for the current
  channel. This effect is a ModPlug hack.
- `Pxy` Panning Slide or Fine Panning Slide: Slides the current channel's
  panning position left or right.
- `X5x` Set Panbrello Waveform: Sets the waveform of future Panbrello effects.
  This effect is a ModPlug hack.
- `X9x` Sound Control: Executes a sound control command. This effect is a
  ModPlug hack.
- `XAx` High Offset: Sets the high offset for future 9xx commands. This effect
  is a ModPlug hack.
- `Yxy` Panbrello: Executes panbrello with speed `x` and depth `y` on the
  current note. This effect is a ModPlug hack.
- `Zxx` MIDI Macro: Executes a macro. This effect is a ModPlug hack.
- `\xx` Smooth MIDI Macro: Executes an interpolated MIDI Macro. This effect is a
  ModPlug hack.
- `#xx` Parameter Extension: ModPlug hack.
