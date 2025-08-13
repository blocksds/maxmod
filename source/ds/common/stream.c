// SPDX-License-Identifier: ISC
//
// Copyright (c) 2008, Mukunda Johnson (mukunda@maxmod.org)
// Copyright (c) 2023, Lorenzooone (lollo.lollo.rbiz@gmail.com)
// Copyright (c) 2025, Antonio Niño Díaz (antonio_nd@outlook.com)

#include <nds.h>

#ifdef ARM9
#include <maxmod9.h>
#else
#include <maxmod7.h>
#endif
#include <mm_types.h>

#include "ds/common/stream.h"

// Is this correct?! Or is it supposed to be the same as "BUS_SPEED"???
#define CLOCK           33554432
#define DELAY_SAMPLES   16

#define TIMER_SPEED     1024
#define TIMER_AUTO      0xC3
#define TIMER_MANUAL    0x83
#define TIMER_DISABLE   0

#define MAX_VOLUME      127
#define LEFT_PANNING    0
#define RIGHT_PANNING   127
#define CENTER_PANNING  64

#define START_CHANNEL_LOOP 0x88

#define CHANNEL_LEFT    4
#define CHANNEL_CENTER  4
#define CHANNEL_RIGHT   5

#define NUM_TIMERS      4

#ifdef ARM7

static uint32_t previous_irq_state;

// Function to disable interrupts via the status register
static void mmSuspendIRQ_t(void)
{
    uint32_t base_cpsr_state = getCPSR();
    previous_irq_state = base_cpsr_state & CPSR_FLAG_IRQ_DIS;
    setCPSR(base_cpsr_state | CPSR_FLAG_IRQ_DIS);
}

// Function to enable interrupts via the status register
static void mmRestoreIRQ_t(void)
{
    setCPSR(previous_irq_state | (getCPSR() & (~CPSR_FLAG_IRQ_DIS)));
}

#endif // ARM7

static mm_hword mmsPreviousTimer;
static mm_word StreamCounter;
static mm_stream_data mmsData;

// Checks if a format is stereo or not
static mm_bool is_stereo_format(mm_stream_formats format)
{
#ifdef MM_SUPPORT_4BIT_STREAM
    return (format == MM_STREAM_8BIT_STEREO) || (format == MM_STREAM_16BIT_STEREO) || (format == MM_STREAM_4BIT_STEREO);
#else
    return (format == MM_STREAM_8BIT_STEREO) || (format == MM_STREAM_16BIT_STEREO);
#endif
}

// Checks if a format is 16 bit or not
static mm_bool is_16bit_format(mm_stream_formats format)
{
    return (format == MM_STREAM_16BIT_MONO) || (format == MM_STREAM_16BIT_STEREO);
}

// Checks if a format is 8 bit or not
//static mm_bool is_8bit_format(mm_stream_formats format)
//{
//    return (format == MM_STREAM_8BIT_MONO) || (format == MM_STREAM_8BIT_STEREO);
//}

#ifdef MM_SUPPORT_4BIT_STREAM
// Checks if a format is 4 bit or not
static mm_bool is_4bit_format(mm_stream_formats format)
{
    return (format == MM_STREAM_4BIT_MONO) || (format == MM_STREAM_4BIT_STEREO);
}
#endif

// Gets a format's shift
static mm_byte get_shift_for_format(mm_stream_formats format)
{
    switch (format)
    {
        case MM_STREAM_16BIT_MONO:
        case MM_STREAM_16BIT_STEREO:
            return 2;
#ifdef MM_SUPPORT_4BIT_STREAM
        case MM_STREAM_4BIT_MONO:
        case MM_STREAM_4BIT_STEREO:
            return 0;
#endif
        case MM_STREAM_8BIT_MONO:
        case MM_STREAM_8BIT_STEREO:
        default:
            return 1;
    }
}

#ifdef ARM7
// Gets a format's value for cnt
static mm_byte get_cnt_format(mm_stream_formats format)
{
    switch (format)
    {
        case MM_STREAM_16BIT_MONO:
        case MM_STREAM_16BIT_STEREO:
            return 1;
#ifdef MM_SUPPORT_4BIT_STREAM
        case MM_STREAM_4BIT_MONO:
        case MM_STREAM_4BIT_STEREO:
            return 2;
#endif
        case MM_STREAM_8BIT_MONO:
        case MM_STREAM_8BIT_STEREO:
        default:
            return 0;
    }
}
#endif

// Copy mono data with different shifts
// It may be better to keep everything separate for faster speeds?!
static ARM_CODE mm_byte *CopyDataMonoStream(mm_byte *work_memory_ptr, mm_word dest_pos,
                                            mm_word curr_samples_num, mm_word shift)
{
    mm_byte* wave_memory_ptr = (mm_byte*)mmsData.wave_memory;

    wave_memory_ptr += (dest_pos << shift) >> 1;

    if (shift == 0)
    {
        // Copy 2 bytes at a time
        for (mm_word i = 0; i < (curr_samples_num >> 2); i++)
        {
            *((mm_hword*)wave_memory_ptr) = *((mm_hword*)work_memory_ptr);
            wave_memory_ptr += 2;
            work_memory_ptr += 2;
        }
    }
    else
    {
        // Copy 4 bytes at a time
        for (mm_word i = 0; i < (curr_samples_num >> (2 - (shift - 1))); i++)
        {
            *((mm_word*)wave_memory_ptr) = *((mm_word*)work_memory_ptr);
            wave_memory_ptr += 4;
            work_memory_ptr += 4;
        }
    }

    return work_memory_ptr;
}

// Copy stereo data with different shifts
// It may be better to keep everything separate for faster speeds?!
static ARM_CODE mm_byte *CopyDataStereoStream(mm_byte *work_memory_ptr, mm_word dest_pos,
                                              mm_word curr_samples_num, mm_word shift,
                                              mm_word length)
{
    mm_byte *wave_memory_ptr = (mm_byte*)mmsData.wave_memory;
    mm_word left = 0;
    mm_word right = 0;

    wave_memory_ptr += (dest_pos << shift) >> 1;
    // Copy 4 bytes at a time
    for (mm_word i = 0; i < (curr_samples_num >> (2 - shift)); i++)
    {
        if (shift != 2)
        {
            left = (*((mm_word*)work_memory_ptr)) & 0xFF00FF;
            right = ((*((mm_word*)work_memory_ptr)) >> 8) & 0xFF00FF;
            left |= left >> 8;
            right |= right >> 8;
        }
        else
        {
            left = (*((mm_word*)work_memory_ptr)) & 0xFFFF;
            right = ((*((mm_word*)work_memory_ptr)) >> 16) & 0xFFFF;
        }
        *((mm_hword*)wave_memory_ptr) = left;
        *((mm_hword*)(wave_memory_ptr + ((length << shift) >> 1))) = right;
        wave_memory_ptr += 2;
        work_memory_ptr += 4;
    }

    return work_memory_ptr;
}

// Copy/de-interleave data from work buffer into the wave buffer
static ARM_CODE void CopyDataToStream(mm_word num_samples)
{
    // Do nothing if there is no data
    if (num_samples == 0)
        return;

    mm_word position = mmsData.position;
    mm_word length = mmsData.length_cut;
    mm_byte* work_memory_ptr = (mm_byte*)mmsData.work_memory;

    while (num_samples != 0)
    {
        mm_word dest_pos = position;
        mm_word remaining_samples = length - position;
        mm_word curr_samples_num = num_samples;

        if (curr_samples_num >= remaining_samples)
        {
            curr_samples_num = remaining_samples;
            position = 0;
        }
        else
        {
            position += curr_samples_num;
        }

        num_samples -= curr_samples_num;

        if (is_stereo_format(mmsData.format))
            work_memory_ptr = CopyDataStereoStream(work_memory_ptr, dest_pos, curr_samples_num, get_shift_for_format(mmsData.format), length);
        else
            work_memory_ptr = CopyDataMonoStream(work_memory_ptr, dest_pos, curr_samples_num, get_shift_for_format(mmsData.format));
    }

    mmsData.position = position;
}

// Executes the stream update
static void StreamExecuteUpdate(mm_word stream_position)
{
    // Update StreamCounter
    StreamCounter += stream_position;

    while (stream_position != 0)
    {
        mm_word processing_samples = stream_position;

        // Cut to work buffer size
        // TODO: This was only > in the asm, but it could cause issues...?
        if (processing_samples >= mmsData.length_cut)
            processing_samples = mmsData.length_cut - 1;

        // Do callback
        mm_word filled_samples = mmsData.callback(processing_samples, mmsData.work_memory, mmsData.format);

        // Prevent bad filling...?
        //if(filled_samples > processing_samples)
        //    filled_samples = processing_samples;

        // Copy samples to stream
        CopyDataToStream(filled_samples);

        // Processed samples
        stream_position -= filled_samples;

        // Break if 0 samples output or remaining < amount filled
        if ((filled_samples == 0) || (stream_position < filled_samples))
        {
            mmsData.remainder += stream_position * mmsData.clocks;
            break;
        }
    }

#ifdef ARM9
    DC_FlushRange(mmsData.wave_memory, mmsData.length_words * 4);
#endif
}

// Force a data request
static void ForceStreamRequest(mm_word num_samples)
{
    StreamExecuteUpdate((num_samples >> 2) << 2);
}

#ifdef ARM7
void mmStreamOpen(mm_stream *stream, mm_addr wavebuffer, mm_addr workbuffer)
{
#else
void mmStreamOpen(mm_stream *stream)
{
#endif
    // Check if it has already been opened
    if (mmsData.is_active)
        return;

#ifdef ARM7
    // Save args if ARM7
    mmsData.wave_memory = wavebuffer;
    mmsData.work_memory = workbuffer;
#endif

    // Check bad timer selection
    if (stream->timer >= NUM_TIMERS)
        return;

    // Check bad rate (for division)
    if (stream->sampling_rate == 0)
        return;

    // Set active
    mmsData.is_active = 1;

    // Calc hwtimer address
    // hw_timer_num did not exist, but it can save writing some annoying code
    mmsData.hw_timer_num = stream->timer;
    mmsData.hw_timer = &TIMER_DATA(mmsData.hw_timer_num);

    // Reset timer
    mmsData.hw_timer[0] = TIMER_DISABLE;
    mmsData.hw_timer[1] = TIMER_DISABLE;

    // Setup IRQ vector
    irqSet(IRQ_TIMER(mmsData.hw_timer_num), mmStreamUpdate);
    irqEnable(IRQ_TIMER(mmsData.hw_timer_num));

    // Calc Clocks (must be divisible by 2 for SOUND)
    mmsData.clocks = ((CLOCK / stream->sampling_rate) >> 1) << 1;

    // Copy length cut to multiple of 16
    mmsData.length_cut = (stream->buffer_length >> 4) << 4;

    // Copy format
    mmsData.format = (mm_stream_formats)stream->format;

    mm_word length = mmsData.length_cut;

    // Shift left if stereo
    if (is_stereo_format(mmsData.format))
        length <<= 1;

    // Shift left if 16 bit
    if (is_16bit_format(mmsData.format))
        length <<= 1;

    // Shift right if 4 bit
#ifdef MM_SUPPORT_4BIT_STREAM
    if (is_4bit_format(mmsData.format))
         length >>= 1;
#endif

    // Save real length (words)
    mmsData.length_words = length >> 2;

#ifdef ARM9
    // Allocate memory on the ARM9
    mmsData.wave_memory = calloc(length, 1);
    mmsData.work_memory = calloc(length, 1);
#endif

    // Handle malloc failure, in general
    if ((mmsData.wave_memory == NULL) || (mmsData.work_memory == NULL))
    {
        mmsData.is_active = 0;
#ifdef ARM9
        free(mmsData.wave_memory);
        free(mmsData.work_memory);
#endif
        return;
    }

    // Reset wave memory
    for (int i = 0; i < mmsData.length_words; i++)
        ((mm_word*)mmsData.wave_memory)[i] = 0;

    // Copy function
    mmsData.callback = stream->callback;

    // Clear remainder
    mmsData.remainder = 0;

    // Reset position
    mmsData.position = 0;

    // Copy is_auto info
    mmsData.is_auto = !stream->manual;

    mmsData.timer = ((mmsData.clocks * mmsData.length_cut) / 2) / TIMER_SPEED;

    mmsPreviousTimer = 0;

    // Force-fill stream with initial data
    ForceStreamRequest(mmsData.length_cut - DELAY_SAMPLES);

    // Reset stream counter
    StreamCounter = 0;

    //mmSuspendIRQ_t();

    mmStreamBegin(mmsData.wave_memory, mmsData.clocks >> 1, mmsData.length_cut, mmsData.format);

    // Start timer
    if (mmsData.is_auto)
    {
        mmsData.hw_timer[0] = -mmsData.timer;
        mmsData.hw_timer[1] = TIMER_AUTO;
    }
    else
    {
        mmsData.hw_timer[0] = 0;
        mmsData.hw_timer[1] = TIMER_MANUAL;
    }

    //mmRestoreIRQ_t();
}

// Utility function which gets and updates (if needed) the number of samples played since the start
static mm_word getAndUpdateStreamPosition(mm_bool update)
{
    mm_word time = mmsData.timer;

    // Manual has different code
    if (!mmsData.is_auto)
    {
        time = mmsData.hw_timer[0];
        mm_word previous_time = mmsPreviousTimer;

        // Handle updating data
        if (update)
            mmsPreviousTimer = time;

        // Handle overflow
        if (time < previous_time)
            time += 1 << 16;
        time -= previous_time;
    }

    mm_word num_samples = (((time * TIMER_SPEED) + mmsData.remainder) / mmsData.clocks);

    if (update)
    {
        // Floors to multiple of 4 and updates remainder
        mmsData.remainder = (((time * TIMER_SPEED) + mmsData.remainder) % mmsData.clocks) + ((num_samples & 3) * mmsData.clocks);
        num_samples = (num_samples >> 2) << 2;
    }

    return num_samples;
}

// Get number of samples that have played since start.
// 32-bit variable overflows every ~36 hours @ 32khz...
mm_word mmStreamGetPosition(void)
{
    // Catch inactive stream
    if (!mmsData.is_active)
        return 0;

    // Catch auto mode
    // (Only manual mode supported)
    if (mmsData.is_auto)
        return 0;

    return StreamCounter + getAndUpdateStreamPosition(0);
}

// Update stream with new data
void mmStreamUpdate(void)
{
    // Catch inactive stream
    if (!mmsData.is_active)
        return;

    // Determine how many samples to mix
    StreamExecuteUpdate(getAndUpdateStreamPosition(1));
}

// Close audio stream
void mmStreamClose(void)
{
    // Catch inactive stream
    if (!mmsData.is_active)
        return;

    // Disable hardware timer
    mmsData.hw_timer[1] = TIMER_DISABLE;

    // Disable irq
    irqDisable(IRQ_TIMER(mmsData.hw_timer_num));

    // Disable system
    mmsData.is_active = 0;
    mmsData.is_auto = 0;
    mmStreamEnd();

#ifdef ARM9
    // Free malloc'd memory
    free(mmsData.work_memory);
    free(mmsData.wave_memory);
#endif
}

#ifdef ARM7

static void init_sound_channel(mm_byte channel, mm_byte panning, uintptr_t wave_memory)
{
    // Clear cnt, tmr and pnt
    REG_SOUNDXCNT(channel) = 0;
    REG_SOUNDXTMR(channel) = 0;
    REG_SOUNDXPNT(channel) = 0;

    // Copy src and tmr
    REG_SOUNDXSAD(channel) = wave_memory;
    REG_SOUNDXTMR(channel) = -mmsData.clocks;

    // Set length
    REG_SOUNDXLEN(channel) = mmsData.length_words;

    // Set volume and panning
    REG_SOUNDXVOL(channel) = MAX_VOLUME;
    REG_SOUNDXPAN(channel) = panning;
}

static void start_sound_channel(mm_byte channel)
{
    REG_SOUNDXCNT(channel) |= (START_CHANNEL_LOOP | (get_cnt_format(mmsData.format) << 5)) << 24;
}

static void stop_sound_channel(mm_byte channel)
{
    REG_SOUNDXCNT(channel) = 0;

    // Unlock channel
    mmUnlockChannels(1 << channel);
}

// Begin audio stream
void mmStreamBegin(mm_addr wave_memory, mm_hword clks, mm_hword len, mm_stream_formats format)
{
    mmsData.wave_memory = wave_memory;
    mmsData.format = format;
    mmsData.clocks = clks;
    mmsData.length_cut = len;
    mmsData.length_words = mmsData.length_cut >> (3 - get_shift_for_format(mmsData.format));

    // Lock channels 4 if stereo isn't set, 4 & 5 otherwise
    // Then, set the channels up
    if (is_stereo_format(mmsData.format))
    {
        mmLockChannels((1 << CHANNEL_LEFT) | (1 << CHANNEL_RIGHT));
        init_sound_channel(CHANNEL_LEFT, LEFT_PANNING, (uintptr_t)mmsData.wave_memory);
        init_sound_channel(CHANNEL_RIGHT, RIGHT_PANNING, ((uintptr_t)mmsData.wave_memory) + (mmsData.length_words << 2));
        mmsData.length_words <<= 1;
    }
    else
    {
        mmLockChannels(1 << CHANNEL_CENTER);
        init_sound_channel(CHANNEL_CENTER, CENTER_PANNING, (uintptr_t)mmsData.wave_memory);
    }

    mmSuspendIRQ_t();

    // Start channels
    if (is_stereo_format(mmsData.format))
    {
        start_sound_channel(CHANNEL_LEFT);
        start_sound_channel(CHANNEL_RIGHT);
    }
    else
    {
        start_sound_channel(CHANNEL_CENTER);
    }

    // Send "start" signal
    ((mm_byte*)mmsData.wave_memory)[(mmsData.length_words * 4) - 1] = 0;

    mmRestoreIRQ_t();
}

// End audio stream
void mmStreamEnd(void)
{
    mmSuspendIRQ_t();

    // Stop channels
    if (is_stereo_format(mmsData.format))
    {
        stop_sound_channel(CHANNEL_LEFT);
        stop_sound_channel(CHANNEL_RIGHT);
    }
    else
    {
        stop_sound_channel(CHANNEL_CENTER);
    }

    // Send "stop" signal
    *((mm_byte*)mmsData.wave_memory) += 1;

    mmRestoreIRQ_t();
}

#endif
