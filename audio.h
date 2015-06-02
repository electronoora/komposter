/*
 * Komposter (c) 2010-2011 Jani Halme (Firehawk/TDA)
 *
 * This code is licensed under the GNU General Public
 * License version 2. See file 'doc/LICENSE'.
 *
 * Audio playback and rendering
 *
 * $Rev$
 * $Date$
 */

#ifndef __AUDIO_H__
#define __AUDIO_H__

#include "arch.h"

#define AUDIOBUFFER_LEN	1024

// You may need to use larger playback buffers in
// some Linux machines to cure stuttering sound
//#define AUDIOBUFFER_LEN 4096

// how many buffers to render ahead of playback.
// this and bufferlen above have a direct effect
// on the latency.
#define AUDIO_RENDER_AHEAD	2

#define OUTPUTFREQ 44100

#define AUDIOMODE_MUTE		0
#define AUDIOMODE_COMPOSING	1
#define AUDIOMODE_PATTERNPLAY	2
#define AUDIOMODE_PLAY		3

#define RENDER_STOPPED          0
#define RENDER_START            1
#define RENDER_IN_PROGRESS      2 
#define RENDER_COMPLETE         3
#define RENDER_PLAYBACK         4
#define RENDER_LIVE		5
#define RENDER_LIVE_COMPLETE	6

int audio_initialize(void);
int audio_isplaying(void);
void audio_release(void);
int audio_update(int cs);
int audio_process(short *buffer, long bufferlen);

void audio_loadpatch(int voice, int synth, int patch);
void audio_trignote(int voice, int note);
void audio_resetsynth(int voice);

int audio_exportwav(); //char *filename);

#endif
