/*
 * Komposter
 *
 * Copyright (c) 2010 Noora Halme et al. (see AUTHORS)
 *
 * This code is licensed under the GNU General Public
 * License version 2. See LICENSE for full text.
 *
 * Utility functions for GUI widgets
 *
 */

#ifndef __WIDGETS_H_
#define __WIDGETS_H_

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "arch.h"
#include "constants.h"
#include "font.h"
#include "modules.h"
#include "bezier.h"


// functions for drawing various widgets on the screen
void draw_module(synthmodule *s);
void draw_signal_node(float x, float y, int type, char* label, int align);
void draw_knob(float x, float y, float setting, float r, float g, float b);
void draw_text(char *dbtext, float x, float y, char align);

void draw_patch(float x1, float y1, float x2, float y2);
void draw_patch_control(float x1, float y1, float x2, float y2, float cx1, float cy1, float cx2, float cy2);
void draw_patch_control_hue(float x1, float y1, float x2, float y2, float cx1, float cy1, float cx2, float cy2, int hue);

void draw_button(float x, float y, float size, char *label, int type);
void draw_textbox(float x, float y, float height, float length, char *label, int type);
void draw_hue_picker(float x, float y, float height, float length, int current);
void draw_dimmer(void);
void draw_kboct(float y, float kw, float kh, int oct, int hlkey, int hlkeydown);
void draw_kbhoct(float x, float y, float kw, float kh, int oct, int hlkey, int hlkeydown, char *kbkeys);
void draw_hslider(float x, float y, float width, float height, float pos, float onscreen, float total, int type);
void draw_vslider(float x, float y, float width, float height, float pos, float onscreen, float total, int type);

// tool functions
int hovertest_box(float x, float y, float bx, float by, float height, float width);
int hovertest_hslider(float x, float y, float sx, float sy, float width, float height, float pos, float onscreen, float total);
int hovertest_vslider(float x, float y, float sx, float sy, float width, float height, float pos, float onscreen, float total);
  
void textbox_edit(char *text, unsigned char key, int size);

#endif
