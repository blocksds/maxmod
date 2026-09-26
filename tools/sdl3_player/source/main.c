// SPDX-License-Identifier: ISC
//
// Copyright (c) 2026 Antonio Niño Díaz

#include <stdlib.h>
#include <stdio.h>

#define SDL_MAIN_USE_CALLBACKS 1  // Use the callbacks instead of main()
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include <maxmod_headless.h>

#include "file.h"

#define SAMPLE_RATE (32 * 1024)

static SDL_Window *window = NULL;
static SDL_Renderer *renderer = NULL;
static SDL_AudioStream *stream = NULL;

static int frames = 0;
static void *soundbank_buffer = NULL;
static size_t soundbank_size;

// This function runs once at startup.
SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[])
{
    // Load soundbank
    // --------------

    const char *soundbank_path = "soundbank.bin";

    if (argc == 2)
        soundbank_path = argv[1];

    // Load file
    file_load(soundbank_path, &soundbank_buffer, &soundbank_size);
    if (soundbank_size == 0)
        return SDL_APP_FAILURE;

    if (!mmInitDefault(soundbank_buffer, 20, SAMPLE_RATE))
    {
        printf("mmInitDefault() failed\n");
        return SDL_APP_FAILURE;
    }

    // Setup window and renderer
    // -------------------------

    SDL_SetAppMetadata("Maxmod SDL3 Player", "1.0", "com.blocksds.maxmod.sdl3_player");

    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO))
    {
        SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    if (!SDL_CreateWindowAndRenderer("Maxmod SDL3 player", 640, 480,
                                     SDL_WINDOW_RESIZABLE, &window, &renderer))
    {
        SDL_Log("Couldn't create window/renderer: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    SDL_SetRenderLogicalPresentation(renderer, 640, 480, SDL_LOGICAL_PRESENTATION_LETTERBOX);

    // Setup audio stream
    // ------------------

    SDL_AudioSpec spec;

    spec.channels = 2;
    spec.format = SDL_AUDIO_S8;
    spec.freq = SAMPLE_RATE;

    stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, NULL, NULL);
    if (!stream)
    {
        SDL_Log("Couldn't create audio stream: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    // SDL_OpenAudioDeviceStream starts the device paused.
    SDL_ResumeAudioStreamDevice(stream);

    return SDL_APP_CONTINUE;
}

// This function runs when a new event (mouse input, keypresses, etc) occurs.
SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event)
{
    if (event->type == SDL_EVENT_MOUSE_BUTTON_DOWN)
        mmStart(0, MM_PLAY_ONCE);

    if (event->type == SDL_EVENT_QUIT)
    {
        return SDL_APP_SUCCESS; // End the program, reporting success to the OS
    }

    return SDL_APP_CONTINUE; // Carry on with the program
}

// This function runs once per frame, and is the heart of the program.
SDL_AppResult SDL_AppIterate(void *appstate)
{
    // Generate streamed audio samples
    // -------------------------------

    // Minimum number of samples that we let the application have in the queue
    const int minimum_audio = (SAMPLE_RATE * sizeof(int8_t)) / 2;

    if (SDL_GetAudioStreamQueued(stream) < minimum_audio)
    {
#define NUM_SAMPLES (SAMPLE_RATE / 10)

        static int8_t samples[NUM_SAMPLES * 2] = { 0 };

        // This generates signed samples
        mmFrame(samples, NUM_SAMPLES);

        // Feed the new data to the stream. It will queue at the end.
        SDL_PutAudioStreamData(stream, samples, sizeof(samples));
    }

    // Render screen
    // -------------

    SDL_RenderClear(renderer);
    SDL_RenderPresent(renderer);

    return SDL_APP_CONTINUE; // Carry on with the program
}

// This function runs once at shutdown.
void SDL_AppQuit(void *appstate, SDL_AppResult result)
{
    mmEnd();

    free(soundbank_buffer);

    // SDL will clean up the window/renderer for us.
}
