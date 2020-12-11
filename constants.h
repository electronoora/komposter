/*
 * Komposter
 *
 * Copyright (c) 2010 Noora Halme et al. (see AUTHORS)
 *
 * This code is licensed under the GNU General Public
 * License version 2. See LICENSE for full text.
 *
 * Global constants
 *
 */

// version number defined on build
#define K_COPYRIGHT "copyright (c) 2010 noora halme et al"
#define K_ABOUT1    "modular virtual analog workstation"
#define K_ABOUT2    "for 4k/8k/64k intros"

// set date as version number if not defined by Makefile
#ifndef K_VERSION
#define K_VERSION __DATE__
#endif

// screen size
#define DS_WIDTH 1024
#define DS_HEIGHT 640

// limits for synthesizers
#define MAX_SYNTH 16
#define MAX_MODULES 64
#define MAX_PATCHES 64

// how close to a node must the cursor be to activate it?
#define NODE_PROXIMITY 6

// pattern limits
#define MAX_PATTERN 256
#define MAX_PATTLENGTH 256

// sequencer limits
#define MAX_CHANNELS 16
#define MAX_SONGLEN 768

// audio output frequency
#define OUTPUTFREQ 44100

// highest note and octave shown
#define MAX_OCTAVE 8
#define MAX_NOTE 119
