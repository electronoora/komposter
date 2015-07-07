/*
 * Komposter (c) 2010-2011 Jani Halme (Firehawk/TDA)
 *
 * This code is licensed under the GNU General Public
 * License version 2. See file 'doc/LICENSE'.
 *
 * Patch editor page 
 *
 * $Rev$
 * $Date$
 */

#ifndef __PATCH_H__
#define __PATCH_H__

#include <stdio.h>
#include "arch.h"
#include "constants.h"
#include "font.h"
#include "modules.h"
#include "synthesizer.h"
#include "widgets.h"

void patch_init(void);

// pattern mouse functions
void patch_mouse_hover(int x, int y);
void patch_mouse_drag(int x, int y);
void patch_mouse_click(int button, int state, int x, int y);
void patch_keyboard(unsigned char key, int x, int y);
void patch_keyboardup(unsigned char key, int x, int y);
void patch_specialkey(unsigned char key, int x, int y);
void patch_draw(void);

// dialog callbacks for the modulator edit box
void patch_draw_modulator(void);
void patch_modulator_hover(int x, int y);
void patch_modulator_click(int button, int state, int x, int y);
void patch_modulator_keyboard(unsigned char key, int x, int y);
void patch_modulator_special(int key, int x, int y);

// conversions to/from scale values 
float knob_scale2float(int scale, float value);
float knob_float2scale(int scale, float value);
                      
#endif
