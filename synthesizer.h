/*
 * Komposter (c) 2010-2011 Jani Halme (Firehawk/TDA)
 *
 * This code is licensed under the GNU General Public
 * License version 2. See file 'doc/LICENSE'.
 *
 * Synthesizer editor page
 *
 * $Rev$
 * $Date$
 */

#ifndef __SYNTHESIZER_H__
#define __SYNTHESIZER_H__

#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <unistd.h>

#include "arch.h"
#include "audio.h"
#include "buffermm.h"
#include "constants.h"
#include "console.h"
#include "dialog.h"
#include "dotfile.h"
#include "filedialog.h"
#include "fileops.h"
#include "modules.h"
#include "widgets.h"
//#include "synthesizer_file.h"

void synth_lockaudio(void);
void synth_releaseaudio(void);

void synth_clear(int csyn);

int hovertest_module(int x, int y, synthmodule *list, int count);
int hovertest_output(int x, int y, synthmodule *list, int count);
int hovertest_input(int x, int y, synthmodule *mod);


void resetactive(synthmodule *list, int count);

int getactive(synthmodule *list, int count);
int getactiveout(synthmodule *list, int count);       
int getactivein(synthmodule *list, int count);

// kb&mouse functions for synthesizer page
void synth_mouse_hover(int x, int y);
void synth_mouse_drag(int x, int y);
void synth_mouse_click(int button, int state, int x, int y);
void synth_keyboard(unsigned char key, int x, int y);
void synth_specialkey(unsigned char key, int x, int y);

// draw functions for synthesizer page
void synth_draw(void);

// modify the synth data
void synth_init(void);
void synth_addmodule(int type);
void synth_deletemodule(int m);


// callbacks for the add module dialog
void synth_draw_addmodule(void);
void synth_addmodule_hover(int x, int y);
void synth_addmodule_click(int button, int state, int x, int y);
void synth_addmodule_keyboard(unsigned char key, int x, int y);


// file dialog functions
void synth_draw_file(void);
void synth_file_hover(int x, int y);
void synth_file_click(int button, int state, int x, int y);
void synth_file_keyboard(unsigned char key, int x, int y);
void synth_file_drag(int x, int y);
void synth_file_checkstate(void);

// functions for converting the module graph into a stack according to signal flow
void synth_stackify(int syn);
int synth_trace(int syn, int pm, int fifo);


// label edit dialog
void synthlabel_draw(void);
void synthlabel_hover(int x, int y);
void synthlabel_click(int button, int state, int x, int y);
void synthlabel_keyboard(unsigned char key, int x, int y);

#endif
