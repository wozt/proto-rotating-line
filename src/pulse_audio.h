/* SPDX-License-Identifier: MIT */
#ifndef PROTO_PULSE_AUDIO_H
#define PROTO_PULSE_AUDIO_H
#include <stdbool.h>
#include <SDL3/SDL.h>
/* SDL3 queued floating-point PCM; sound events are < 0.25 s. */
SDL_AudioStream *proto_pulse_audio_open(void);
void proto_pulse_audio_emit(SDL_AudioStream *audio, bool aria);
#endif
