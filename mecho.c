/* so yeah it's a slightly modified sdl example intended to create echo from the microphone with a specified delay because reasons */

/*
  Copyright (C) 1997-2016 Sam Lantinga <slouken@libsdl.org>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely.
*/
#include "SDL2/SDL.h"

#include <stdlib.h>

static SDL_Window *window = NULL;
static SDL_Renderer *renderer = NULL;
static SDL_AudioSpec spec;
static SDL_AudioDeviceID devid_in = 0;
static SDL_AudioDeviceID devid_out = 0;

static Uint32 startplayback(Uint32 interval, void *param)
{
	SDL_Event event;
	SDL_UserEvent userevent;

	/* In this example, our callback pushes an SDL_USEREVENT event
	   into the queue, and causes our callback to be called again at the
	   same interval: */

	userevent.type = SDL_USEREVENT;
	userevent.code = 0;
	userevent.data1 = NULL;
	userevent.data2 = NULL;

	event.type = SDL_USEREVENT;
	event.user = userevent;

	SDL_PushEvent(&event);
	return(interval);
}

static void
loop()
{
    SDL_bool please_quit = SDL_FALSE;
    SDL_Event e;

    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) {
            please_quit = SDL_TRUE;
        } else if (e.type == SDL_KEYDOWN) {
            if (e.key.keysym.sym == SDLK_ESCAPE) {
                please_quit = SDL_TRUE;
            }
	} else if (e.type == SDL_USEREVENT) {
		SDL_PauseAudioDevice(devid_out, SDL_FALSE);
	}
    }

    if (SDL_GetAudioDeviceStatus(devid_in) == SDL_AUDIO_PLAYING) {
        SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
    } else {
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
    }

    SDL_RenderClear(renderer);
    SDL_RenderPresent(renderer);

    if (please_quit) {
        /* stop playing back, quit. */
        SDL_Log("Shutting down.\n");
        SDL_PauseAudioDevice(devid_in, 1);
        SDL_CloseAudioDevice(devid_in);
        SDL_PauseAudioDevice(devid_out, 1);
        SDL_CloseAudioDevice(devid_out);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        exit(0);
    }

    /* Note that it would be easier to just have a one-line function that
        calls SDL_QueueAudio() as a capture device callback, but we're
        trying to test the API, so we use SDL_DequeueAudio() here. */
    while (SDL_TRUE) {
        Uint8 buf[1024];
        const Uint32 br = SDL_DequeueAudio(devid_in, buf, sizeof (buf));
        SDL_QueueAudio(devid_out, buf, br);
        if (br < sizeof (buf)) {
            break;
        }
    }
}

int
main(int argc, char **argv)
{
    /* (argv[1] == NULL means "open default device.") */
    const char *devname = NULL;
    SDL_AudioSpec wanted;
    int devcount;
    int i;
    int delay;

    if (argc > 3 || argc < 2) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "mecho delay(ms) [device]\n");
        return (1);
    }
    if (argc == 3)
	    devname = argv[2];
    delay = atoi (argv[1]);

    /* Enable standard application logging */
    SDL_LogSetPriority(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_INFO);

    /* Load the SDL library */
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't initialize SDL: %s\n", SDL_GetError());
        return (1);
    }

    window = SDL_CreateWindow("testaudiocapture", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 320, 240, 0);
    renderer = SDL_CreateRenderer(window, -1, 0);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    SDL_RenderPresent(renderer);

    SDL_Log("Using audio driver: %s\n", SDL_GetCurrentAudioDriver());

    devcount = SDL_GetNumAudioDevices(SDL_TRUE);
    for (i = 0; i < devcount; i++) {
        SDL_Log(" Capture device #%d: '%s'\n", i, SDL_GetAudioDeviceName(i, SDL_TRUE));
    }

    SDL_zero(wanted);
    wanted.freq = 44100;
    wanted.format = AUDIO_F32SYS;
    wanted.channels = 1;
    wanted.samples = 4096;
    wanted.callback = NULL;

    SDL_zero(spec);

    /* DirectSound can fail in some instances if you open the same hardware
       for both capture and output and didn't open the output end first,
       according to the docs, so if you're doing something like this, always
       open your capture devices second in case you land in those bizarre
       circumstances. */

    SDL_Log("Opening default playback device...\n");
    devid_out = SDL_OpenAudioDevice(NULL, SDL_FALSE, &wanted, &spec, SDL_AUDIO_ALLOW_ANY_CHANGE);
    if (!devid_out) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't open an audio device for playback: %s!\n", SDL_GetError());
        SDL_Quit();
        exit(1);
    }

    SDL_Log("Opening capture device %s%s%s...\n",
            devname ? "'" : "",
            devname ? devname : "[[default]]",
            devname ? "'" : "");

    devid_in = SDL_OpenAudioDevice(devname, SDL_TRUE, &spec, &spec, 0);
    if (!devid_in) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't open an audio device for capture: %s!\n", SDL_GetError());
        SDL_Quit();
        exit(1);
    }

    SDL_Log("Ready! Hold down mouse or finger to record!\n");

    SDL_PauseAudioDevice(devid_in, SDL_FALSE);
    SDL_PauseAudioDevice(devid_out, SDL_TRUE);

    SDL_AddTimer(delay, startplayback, NULL);
    while (1) { loop(); SDL_Delay(100); }

    return 0;
}
