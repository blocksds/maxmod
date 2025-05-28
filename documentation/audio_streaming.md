# Audio Streaming

Maxmod has a special capability to allow the user to stream customized data to
the sound channels. This is called *audio streaming*.

## Stream Information

Before you open the stream, you must understand the **mm_stream** structure.

```c
typedef struct t_mmstream
{
    mm_word sampling_rate;
    mm_word buffer_length;
    mm_stream_func callback;
    mm_stream_formats format;
    mm_stream_timer timer;
    mm_bool manual;
} mm_stream;
```

This structure contains the neccesary information about how the stream will
operate.

### sampling\_rate

This is the sampling rate of the stream in Hertz. Higher rates will provide
better quality, but also more frequent requests to your filling routine. Pick
according to which rate your filling routine will output.

### buffer\_length

This is the length of the wave buffer(s). It is specified in samples. You should
make this a multiple of 16.

### callback

This is a pointer to your filling routine. Your routine is responsible for
handling fill requests from Maxmod.

### format

This entry controls the output format for the stream. There are 4 settings which
are listed in **mm_stream_formats**.

```c
typedef enum
{
    MM_STREAM_8BIT_MONO      = 0x0, // 000b
    MM_STREAM_8BIT_STEREO    = 0x1, // 001b

    MM_STREAM_16BIT_MONO     = 0x2, // 010b
    MM_STREAM_16BIT_STEREO   = 0x3, // 011b

} mm_stream_formats;
```

### timer

This entry controls which timer will be used for counting samples. Choose one of
the 4 hardware timers available on ARM9.

```c
typedef enum
{
    MM_TIMER0, // Hardware timer 0
    MM_TIMER1, // Hardware timer 1
    MM_TIMER2, // Hardware timer 2
    MM_TIMER3  // Hardware timer 3

} mm_stream_timer;
```

If your program disturbs the streaming timer while it is in use, then the stream
will be corrupted.

### manual

If this entry is *true* then the requests to your filling routine will not be
automatic. You will then have to call mmStreamUpdate() manually to keep the
stream filled. This is useful if you want to control when the stream will be
filled.

## Opening the Stream

Here is an example to open a monoural stream with 22KHz playback rate and 8-bit
output.

```c
void open_stream(void)
{
    mm_stream stream;

    stream.sampling_rate = 22000;        // 22khz
    stream.buffer_length = 800;          // Should be adequate
    stream.callback = fill_stream;       // Give fill routine
    stream.format = MM_STREAM_8BIT_MONO; // 8-bit mono
    stream.timer = MM_TIMER0;            // Use timer 0
    stream.manual = 0;                   // Auto filling

    mmOpenStream(&stream);
}
```

And... another example that opens a stereo stream with 16KHz playback rate and
16-bit output.

```c
void open_stream(void)
{
    mm_stream stream;

    stream.sampling_rate = 16384;           // 16KHZ
    stream.buffer_length = 800;             // Should be adequate
    stream.callback = my_stereo_filler;     // Give stereo filling routine
    stream.format = MM_STREAM_16BIT_STEREO; // Select format
    stream.timer = MM_TIMER0;               // Use timer 0
    stream.manual = 0;                      // Auto filling

    // open the stream
    mmOpenStream(&stream);
}
```

Try to understand what's going on in these two examples by using the information
explained above.

### Caution

When opening the stream when music/sound is playing, some notes may be cut due
to being interrupted by the starting of the stream. (if they happen to occupy
the channels used for streaming)

## Filling the Stream

After you open the stream (in auto mode), Maxmod will start sending you *fill*
requests. These requests are handled by the function you specified in the
callback entry.

Here is the format for the callback function.

```c
typedef mm_word (* mm_stream_func)(mm_word length, mm_addr dest, mm_stream_formats format);
```

The **dest** parameter contains the address that you must output the new data
to. **length** is the (recommended) amount of samples that should be output to
the target. **format** is a copy of the value that you specified during setup.
The return value of the callback is how many samples you have actually output.
If you return a value less than 'length', then the remaining samples will be
saved for the next stream update.

Here is a filling routine to output white noise (for the mono opening example
above).

```c
mm_word fill_stream(mm_word length, mm_addr dest, mm_stream_formats format)
{
    s8 *output = (s8 *)dest;
    for ( ; length; length--)
    {
        *output++ = rand();
    }
    return length;
}
```

And... an example to fill the stereo output with white noise (16-bit).

```c
mm_word my_stereo_filler(mm_word length, mm_addr dest, mm_stream_formats format)
{
    s16 *output = (s16 *)dest;
    for ( ; length; length--)
    {
        *output++ = rand(); // Output left sample
        *output++ = rand(); // Output right sample (data is interleaved)
    }
    return length;
}
```

With these examples combined with the ones above, white noise should be produced
through the speakers. Note that when using automatic filling you should always
output the full 'length' amount unless you really know what you are doing.

Another note: The callback will only be called with a length that is a multiple
of 4. **The amount that you output must also be a multiple of 4** (zero is okay,
to skip the update), the internal routines are optimized with this in mind and
they do not handle situations where the callback returns an amount that is not a
multiple of 4.

## Manual Filling

Sometimes, when your filling code depends on certain data that is edited during
your program logic, it is undesirable to have the filling routine called with
*random* requests.

If this is the case, you need to switch the stream to **manual** mode. Specify a
nonzero value in the *manual* entry of the stream information.

In this mode, you are responsible for issuing fill requests. The
**mmStreamUpdate()** will refill the stream up to the point that is currently
being played.

Here is an example program loop that will fill the stream after the program
logic executes.

```c
while (true)
{
    // ... Update game logic ...

    // Fill the stream with data
    mmUpdateStream();

    // Wait for new frame
    swiWaitForVBlank();

    // ... Update graphical data ...
}
```

Notice that this program only calls **mmStreamUpdate()** once per frame. You
must ensure that the size of the wave buffer is able to hold enough data to be
played until the next fill request.

## ARM7/ARM9

The streaming functions can be used on both processors. If **mmStreamOpen()** is
used on ARM9, then the ARM9 will be responsible for filling the stream, and the
hardware timers on the ARM9 side will be used for timing. If **mmStreamOpen()**
is called on the ARM7, then the ARM7 will be responsible to fill the wave
buffer, and the ARM7 timers will be used.

## Ending the Stream

When you want to stop the stream, call **mmStreamClose()**.

## Closing

For a better understanding, please have a look at the streaming example source
code.
