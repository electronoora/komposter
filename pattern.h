/*
 * Komposter (c) 2010-2011 Jani Halme (Firehawk/TDA)
 *
 * This code is licensed under the GNU General Public
 * License version 2. See file 'doc/LICENSE'.
 *
 * Pattern editor page
 *
 * $Rev$
 * $Date$
 */

#ifndef __PATTERN_H__
#define __PATTERN_H__

#include <stdio.h>
#include "arch.h"
#include "audio.h"
#include "console.h"
#include "constants.h"
#include "font.h"
#include "widgets.h"

// flags for notes
#define NOTE_LEGATO     0x0100 // tie notes together, ie. gate stays up continuously
#define NOTE_ACCENT     0x0200 // accent the note - synth determines the effect
#define NOTE_STACCATO   0x0400 // drop gate with env trig at the end of attack (not implemented)
#define NOTE_PORTAMENTO 0x0800 // slide to this note in the given duration (not implemented)

void pattern_init(void);

// pattern mouse functions
void pattern_mouse_hover(int x, int y);
void pattern_mouse_drag(int x, int y);
void pattern_mouse_click(int button, int state, int x, int y);
void pattern_keyboard(unsigned char key, int x, int y);
void pattern_keyboardup(unsigned char key, int x, int y);
void pattern_specialkey(unsigned char key, int x, int y);

void pattern_draw(void);

#endif
