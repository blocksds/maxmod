# Hardware Usage

It is important to understand which hardware resources Maxmod uses while it is
operating. Modifying certain hardware registers while they are being used by
Maxmod can, in worst case, lead to a crash.

## GBA Hardware Usage

### Timer 0

Maxmod uses Timer 0 as the sampling timer for both output channels. Modifying it
while it is in use will cause audio corruption.

### DMA 1 and 2

Maxmod uses DMA 1 and 2 for transferring waveform data to the Direct Sound FIFO.

### VBlank IRQ

The VBlank IRQ is used to reset DMA transfers and swap mixing buffers. The
timing is extremely critical, so make sure the the handler does not get
interrupted. If you want to have your own handler called after the routine is
completed, install your handler with mmSetVBlankHandler.

### Direct Sound Registers

Maxmod takes control of the Direct Sound registers. You can still use the GB/GBC
synthensizer, but you have to be careful not to disturb the Direct Sound options
(some of the GBC sound registers are shared with the Direct Sound registers).

## DS Hardware Usage

### DS Sound Registers (ARM7)

Maxmod has direct control over the DS sound registers on the ARM7 side. If you
need to use certain hardware channels for some reason, use mmLockChannels to
disable Maxmod from using them.

### Update timer (ARM7)

Timer `LIBNDS_DEFAULT_TIMER_MUSIC` on ARM7 is used by Maxmod to time update
events.

### Streaming timer (ARM9)

If you enable audio streaming, one timer is used to count output samples. The
timer that is used can be chosen.
