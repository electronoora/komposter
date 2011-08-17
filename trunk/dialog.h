/*
 * Komposter (c) 2010-2011 Jani Halme (Firehawk/TDA)
 *
 * This code is licensed under the GNU General Public
 * License version 2. See file 'doc/LICENSE'.
 *
 * Framework for modal dialog windows
 *
 * $Rev$
 * $Date$
 */

#ifndef __DIALOG_H__
#define __DIALOG_H__

#include "widgets.h"

int is_dialog(void);
int is_dialogkb(void);
int is_dialogdrag(void);

void dialog_open(void *draw, void *hover, void *click);
void dialog_bindkeyboard(void *kbfunc);
void dialog_close(void);

void dialog_draw(void);
void dialog_hover(int x, int y);
void dialog_click(int button, int state, int x, int y);
void dialog_keyboard(unsigned char key, int x, int y);
void dialog_binddrag(void *dragfunc);
void dialog_drag(int x, int y);

#endif
