// SPDX-License-Identifier: ISC
//
// Copyright (c) 2026 Antonio Niño Díaz

#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>

#define SDL_MAIN_USE_CALLBACKS 1  // Use the callbacks instead of main()
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include <maxmod_headless.h>

#include "file.h"

#define WINDOW_WIDTH    256
#define WINDOW_HEIGHT   240

#define SAMPLE_RATE (32 * 1024)

static SDL_Window *window = NULL;
static SDL_Renderer *renderer = NULL;
static SDL_AudioStream *stream = NULL;

static SDL_Texture *texture = NULL;
static int texture_width = 0;
static int texture_height = 0;

static int frames = 0;
static void *soundbank_buffer = NULL;
static size_t soundbank_size;

mm_word module_count;
mm_word sample_count;

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

    module_count = mmGetModuleCount();
    sample_count = mmGetSampleCount();

    // Setup window and renderer
    // -------------------------

    SDL_SetAppMetadata("Maxmod SDL3 Player", "1.0", "com.blocksds.maxmod.sdl3_player");

    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO))
    {
        SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    if (!SDL_CreateWindowAndRenderer("Maxmod SDL3 player",
                                     WINDOW_WIDTH * 3, WINDOW_HEIGHT * 3,
                                     SDL_WINDOW_RESIZABLE, &window, &renderer))
    {
        SDL_Log("Couldn't create window/renderer: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    SDL_SetRenderLogicalPresentation(renderer, WINDOW_WIDTH, WINDOW_HEIGHT,
                                     SDL_LOGICAL_PRESENTATION_LETTERBOX);

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

    // Load font image
    // ---------------

    SDL_Surface *surface = SDL_LoadPNG("default_font.png");
    if (!surface) {
        SDL_Log("Couldn't load png: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    SDL_SetSurfaceColorKey(surface, true, 0x00000000);

    texture_width = surface->w;
    texture_height = surface->h;

    texture = SDL_CreateTextureFromSurface(renderer, surface);
    if (!texture) {
        SDL_Log("Couldn't create static texture: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    SDL_DestroySurface(surface); // The texture has a copy of the pixels now

    return SDL_APP_CONTINUE;
}

enum
{
    MENU_START,

    MENU_SFX_ID = MENU_START,
    MENU_SFX_RATE,
    MENU_SFX_VOLUME,
    MENU_SFX_PANNING,

    MENU_MOD_ID,
    MENU_MOD_TEMPO,
    MENU_MOD_PITCH,
    MENU_MOD_VOLUME,

    MENU_END = MENU_MOD_VOLUME,
}
menu_option = MENU_START;

// Values selected by the user from the menu

unsigned int selected_sfx_id = 0;
unsigned int selected_sfx_rate = 1024;
unsigned int selected_sfx_volume = 255;
unsigned int selected_sfx_panning = 128;

unsigned int selected_module_id = 0;
unsigned int selected_module_tempo = 1024;
unsigned int selected_module_pitch = 1024;
unsigned int selected_module_volume = 1024;

// Values related to the sounds currently being played

mm_sfxhand active_sfx_handle = MM_SFXHAND_INVALID;
int active_sfx_id = -1;

int active_module_id = -1;

// This function runs when a new event (mouse input, keypresses, etc) occurs.
SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event)
{
    // Handle keyboard input
    // ---------------------
    if (event->type == SDL_EVENT_KEY_DOWN)
    {
        // If escape is pressed, end program
        if (event->key.scancode == SDL_SCANCODE_ESCAPE)
            return SDL_APP_SUCCESS;

        if (event->key.scancode == SDL_SCANCODE_LEFT)
        {
            if (menu_option == MENU_SFX_ID)
            {
                if (selected_sfx_id == 0)
                    selected_sfx_id = sample_count - 1;
                else
                    selected_sfx_id--;
            }
            else if (menu_option == MENU_SFX_RATE)
            {
                selected_sfx_rate -= 16;

                if (selected_sfx_rate < 512)
                    selected_sfx_rate = 512;

                if (active_sfx_handle != -1)
                    mmEffectRate(active_sfx_handle, selected_sfx_rate);
            }
            else if (menu_option == MENU_SFX_VOLUME)
            {
                if (selected_sfx_volume > 0)
                    selected_sfx_volume--;

                if (active_sfx_handle != -1)
                    mmEffectVolume(active_sfx_handle, selected_sfx_volume);
            }
            else if (menu_option == MENU_SFX_PANNING)
            {
                if (selected_sfx_panning > 0)
                    selected_sfx_panning--;

                if (active_sfx_handle != -1)
                    mmEffectPanning(active_sfx_handle, selected_sfx_panning);
            }
            else if (menu_option == MENU_MOD_ID)
            {
                if (selected_module_id == 0)
                    selected_module_id = module_count - 1;
                else
                    selected_module_id--;
            }
            else if (menu_option == MENU_MOD_TEMPO)
            {
                selected_module_tempo -= 16;

                if (selected_module_tempo < 512)
                    selected_module_tempo = 512;

                mmSetModuleTempo(selected_module_tempo);
            }
            else if (menu_option == MENU_MOD_PITCH)
            {
                selected_module_pitch -= 16;

                if (selected_module_pitch < 512)
                    selected_module_pitch = 512;

                mmSetModulePitch(selected_module_pitch);
            }
            else if (menu_option == MENU_MOD_VOLUME)
            {
                if (selected_module_volume > 4)
                    selected_module_volume -= 4;
                else
                    selected_module_volume = 0;

                mmSetModuleVolume(selected_module_volume);
            }
        }
        else if (event->key.scancode == SDL_SCANCODE_RIGHT)
        {
            if (menu_option == MENU_SFX_ID)
            {
                selected_sfx_id++;
                if (selected_sfx_id == sample_count)
                    selected_sfx_id = 0;
            }
            else if (menu_option == MENU_SFX_RATE)
            {
                selected_sfx_rate += 16;

                if (selected_sfx_rate > 2048)
                    selected_sfx_rate = 2047;

                if (active_sfx_handle != -1)
                    mmEffectRate(active_sfx_handle, selected_sfx_rate);
            }
            else if (menu_option == MENU_SFX_VOLUME)
            {
                if (selected_sfx_volume < 255)
                    selected_sfx_volume++;

                if (active_sfx_handle != -1)
                    mmEffectVolume(active_sfx_handle, selected_sfx_volume);
            }
            else if (menu_option == MENU_SFX_PANNING)
            {
                if (selected_sfx_panning < 255)
                    selected_sfx_panning++;

                if (active_sfx_handle != -1)
                    mmEffectPanning(active_sfx_handle, selected_sfx_panning);
            }
            else if (menu_option == MENU_MOD_ID)
            {
                selected_module_id++;
                if (selected_module_id == module_count)
                    selected_module_id = 0;
            }
            else if (menu_option == MENU_MOD_TEMPO)
            {
                selected_module_tempo += 16;

                if (selected_module_tempo >= 2048)
                    selected_module_tempo = 2048;

                mmSetModuleTempo(selected_module_tempo);
            }
            else if (menu_option == MENU_MOD_PITCH)
            {
                selected_module_pitch += 16;

                if (selected_module_pitch >= 2048)
                    selected_module_pitch = 2048;

                mmSetModulePitch(selected_module_pitch);
            }
            else if (menu_option == MENU_MOD_VOLUME)
            {
                if (selected_module_volume < (1024 - 4))
                    selected_module_volume += 4;
                else
                    selected_module_volume = 1024;

                mmSetModuleVolume(selected_module_volume);
            }
        }

        if (event->key.scancode == SDL_SCANCODE_DOWN)
        {
            if (menu_option == MENU_END)
                menu_option = MENU_START;
            else
                menu_option++;
        }
        else if (event->key.scancode == SDL_SCANCODE_UP)
        {
            if (menu_option == MENU_START)
                menu_option = MENU_END;
            else
                menu_option--;
        }

        if (event->key.scancode == SDL_SCANCODE_X)
        {
            if ((menu_option == MENU_SFX_ID) || (menu_option == MENU_SFX_RATE) ||
                (menu_option == MENU_SFX_VOLUME) || (menu_option == MENU_SFX_PANNING))
            {
                mmEffectCancel(active_sfx_handle);

                //if (active_sfx_id != -1)
                //    mmUnloadEffect(active_sfx_id);

                active_sfx_id = selected_sfx_id;

                //if (mmLoadEffect(active_sfx_id) != 0)
                //{
                //    printf("Failed to load effect %d", active_sfx_id);
                //    wait_forever();
                //}

                mm_sound_effect effect =
                {
                    .id = active_sfx_id,
                    .rate = selected_sfx_rate,
                    .handle = 0,
                    .volume = selected_sfx_volume,
                    .panning = selected_sfx_panning
                };

                active_sfx_handle = mmEffectEx(&effect);
                if (active_sfx_handle == MM_SFXHAND_INVALID)
                {
                    printf("Failed to play effect %d", active_sfx_id);
                    //wait_forever();
                }
            }
            else if ((menu_option == MENU_MOD_ID) || (menu_option == MENU_MOD_TEMPO) ||
                     (menu_option == MENU_MOD_PITCH) || (menu_option == MENU_MOD_VOLUME))
            {
                mmStop();

                //if (active_module_id != -1)
                //    mmUnload(active_module_id);

                active_module_id = selected_module_id;
                //if (mmLoad(active_module_id) != 0)
                //{
                //    printf("Failed to load module %d", active_module_id);
                //    wait_forever();
                //}

                mmStart(active_module_id, MM_PLAY_LOOP);
                mmSetModuleTempo(selected_module_tempo);
                mmSetModulePitch(selected_module_pitch);
                mmSetModuleVolume(selected_module_volume);
            }
        }
        else if (event->key.scancode == SDL_SCANCODE_Z)
        {
            if ((menu_option == MENU_SFX_ID) || (menu_option == MENU_SFX_RATE) ||
                (menu_option == MENU_SFX_VOLUME) || (menu_option == MENU_SFX_PANNING))
            {
                mmEffectCancel(active_sfx_handle);

                //if (active_sfx_id != -1)
                //    mmUnloadEffect(active_sfx_id);

                active_sfx_id = -1;
                active_sfx_handle = MM_SFXHAND_INVALID;
            }
            else if ((menu_option == MENU_MOD_ID) || (menu_option == MENU_MOD_TEMPO) ||
                     (menu_option == MENU_MOD_PITCH) || (menu_option == MENU_MOD_VOLUME))
            {
                mmStop();

                //if (active_module_id != -1)
                //    mmUnload(active_module_id);

                active_module_id = -1;
            }
        }
    }

    if (event->type == SDL_EVENT_QUIT)
    {
        return SDL_APP_SUCCESS; // End the program, reporting success to the OS
    }

    return SDL_APP_CONTINUE; // Carry on with the program
}

int print_x = 0;
int print_y = 0;

static void print_reset_cursor(void)
{
    print_x = 0;
    print_y = 0;
}

static void print_set_cursor(int x, int y)
{
    print_x = x;
    print_y = y;
}

static void print_format(const char *fmt, ...)
{
    char dest[2000];

    va_list args;
    va_start(args, fmt);
    vsnprintf(dest, sizeof(dest), fmt, args);
    va_end(args);

    dest[sizeof(dest) - 1] = '\0';

    int char_width = texture_width / 32;
    int char_height = texture_height / 8;

    int i = 0;
    while (1)
    {
        char c = dest[i++];

        if (c == '\0')
            break;

        if (c == '\n')
        {
            print_x = 0;
            print_y += char_height;
            continue;
        }

        SDL_FRect src_rect = {
            .x = (c % 32) * char_width,
            .y = (c / 32) * char_height,
            .w = char_width,
            .h = char_height,
        };

        SDL_FRect dst_rect = {
            .x = print_x,
            .y = print_y,
            .w = char_width,
            .h = char_height,
        };

        SDL_RenderTexture(renderer, texture, &src_rect, &dst_rect);

        print_x += char_width;
        if ((print_x + char_width) > WINDOW_WIDTH)
        {
            print_x = 0;
            print_y += char_height;
        }
    }
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

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE); // Black, full alpha
    SDL_RenderClear(renderer);

    {
        SDL_Vertex vertices[6];
        SDL_zeroa(vertices);

        vertices[0].position.x = 0;
        vertices[0].position.y = 0;
        vertices[0].color.b = 0.25;
        vertices[0].color.a = 1.0;

        vertices[1].position.x = WINDOW_WIDTH;
        vertices[1].position.y = 0;
        vertices[1].color.b = 0.25;
        vertices[1].color.a = 1.0;

        vertices[2].position.x = WINDOW_WIDTH;
        vertices[2].position.y = WINDOW_HEIGHT;
        vertices[2].color.b = 0.75;
        vertices[2].color.a = 1.0;

        vertices[3].position.x = 0;
        vertices[3].position.y = 0;
        vertices[3].color.b = 0.25;
        vertices[3].color.a = 1.0;

        vertices[4].position.x = WINDOW_WIDTH;
        vertices[4].position.y = WINDOW_HEIGHT;
        vertices[4].color.b = 0.75;
        vertices[4].color.a = 1.0;

        vertices[5].position.x = 0;
        vertices[5].position.y = WINDOW_HEIGHT;
        vertices[5].color.b = 0.75;
        vertices[5].color.a = 1.0;

        SDL_RenderGeometry(renderer, NULL, vertices, 6, NULL, 0);
    }

    print_reset_cursor();

    print_format("          Maxmod demo\n");
    print_format("          -----------\n");
    print_format("\n");
    print_format("\n");
    print_format("Sample count: %u\n", sample_count);
    print_format("Module count: %u\n", module_count);
    print_format("\n");
    print_format("\n");

#define SEL_OPTION(x) (menu_option == (x) ? '>' : ' ')

    print_format("              [SFX]\n");
    print_format("\n");
    print_format("     %c Sample ID: %u\n", SEL_OPTION(MENU_SFX_ID), selected_sfx_id);
    print_format("     %c Rate:      %u\n", SEL_OPTION(MENU_SFX_RATE), selected_sfx_rate);
    print_format("     %c Volume:    %u\n", SEL_OPTION(MENU_SFX_VOLUME), selected_sfx_volume);
    print_format("     %c Panning:   %u\n", SEL_OPTION(MENU_SFX_PANNING), selected_sfx_panning);
    print_format("\n");
    print_format("            [Module]\n");
    print_format("\n");
    print_format("     %c Module ID: %u\n", SEL_OPTION(MENU_MOD_ID), selected_module_id);
    print_format("     %c Tempo:     %u\n", SEL_OPTION(MENU_MOD_TEMPO), selected_module_tempo);
    print_format("     %c Pitch:     %u\n", SEL_OPTION(MENU_MOD_PITCH), selected_module_pitch);
    print_format("     %c Volume:    %u\n", SEL_OPTION(MENU_MOD_VOLUME), selected_module_volume);
    print_format("\n");
    print_format("\n");

    if ((menu_option == MENU_SFX_ID) || (menu_option == MENU_SFX_RATE) ||
        (menu_option == MENU_SFX_VOLUME) || (menu_option == MENU_SFX_PANNING))
    {
        print_format("X: Start SFX\n");
        print_format("Z: Stop SFX\n");
    }
    else if ((menu_option == MENU_MOD_ID) || (menu_option == MENU_MOD_TEMPO) ||
        (menu_option == MENU_MOD_PITCH) || (menu_option == MENU_MOD_VOLUME))
    {
        print_format("X: Start module\n");
        print_format("Z: Stop module\n");
    }
    else
    {
        print_format("\n");
        print_format("\n");
    }
    print_format("\n");
    print_format("UP/DOWN:    Move cursor\n");
    print_format("LEFT/RIGHT: Change value\n");

    print_format("\n");
    print_format("ESC: Close program\n");
    print_format("\n");

    SDL_RenderPresent(renderer);

    return SDL_APP_CONTINUE; // Carry on with the program
}

// This function runs once at shutdown.
void SDL_AppQuit(void *appstate, SDL_AppResult result)
{
    mmEnd();

    free(soundbank_buffer);

    SDL_DestroyTexture(texture);

    // SDL will clean up the window/renderer for us.
}
