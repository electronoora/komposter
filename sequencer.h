/*
 * Komposter
 *
 * Copyright (c) 2010 Noora Halme et al. (see AUTHORS)
 *
 * This code is licensed under the GNU General Public
 * License version 2. See LICENSE for full text.
 *
 * Sequencer page
 *
 */

#ifndef __SEQUENCER_H__
#define __SEQUENCER_H__

#include <stdio.h>
#include "arch.h"
#include "audio.h"
#include "buffermm.h"
#include "constants.h"
#include "dialog.h"
#include "font.h"
#include "patch.h"
#include "synthesizer.h"
#include "widgets.h"

// channel restart flags in seq
#define SEQ_RESTART_ENV	1
#define	SEQ_RESTART_VCO	2
#define SEQ_RESTART_LFO	4


void sequencer_init();

void sequencer_clearsong(void);

int sequencer_ispattern(int ch, int clickpos);
int sequencer_patternstart(int ch, int clickpos);

void sequencer_mouse_hover(int x, int y);
void sequencer_mouse_drag(int x, int y);
void sequencer_mouse_click(int button, int state, int x, int y);
void sequencer_keyboard(unsigned char key, int x, int y);
void sequencer_draw(void);

void sequencer_draw_pattern(void);
void sequencer_pattern_hover(int x, int y);
void sequencer_pattern_click(int button, int state, int x, int y);
void sequencer_pattern_keyboard(unsigned char key, int x, int y);

void sequencer_draw_channel(void);
void sequencer_channel_hover(int x, int y);
void sequencer_channel_click(int button, int state, int x, int y);
void sequencer_channel_keyboard(unsigned char key, int x, int y);

void sequencer_draw_render(void);
void sequencer_render_hover(int x, int y);
void sequencer_render_click(int button, int state, int x, int y);
void sequencer_render_keyboard(unsigned char key, int x, int y);


void sequencer_draw_preview(void);
void sequencer_preview_hover(int x, int y);
void sequencer_preview_click(int button, int state, int x, int y);
void sequencer_preview_keyboard(unsigned char key, int x, int y);


void sequencer_draw_file(void);
void sequencer_file_hover(int x, int y);
void sequencer_file_click(int button, int state, int x, int y);
void sequencer_file_keyboard(unsigned char key, int x, int y);
void sequencer_file_drag(int x, int y);
void sequencer_file_checkstate(void);

void sequencer_draw_bpm(void);
void sequencer_bpm_hover(int x, int y);
void sequencer_bpm_click(int button, int state, int x, int y);
void sequencer_bpm_keyboard(unsigned char key, int x, int y);
int sequencer_bpm_convert(void);
void sequencer_bpm_close_dialog();
    
#endif
