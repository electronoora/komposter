/*
 * Komposter
 *
 * Copyright (c) 2010 Noora Halme et al. (see AUTHORS)
 *
 * This code is licensed under the GNU General Public
 * License version 2. See LICENSE for full text.
 *
 * Info dialog                   
 *
 */

#ifndef __ABOUT_H__
#define __ABOUT_H__

#include "arch.h"
#include "dialog.h"
#include "widgets.h"

void about_draw(void);
void about_hover(int x, int y);
void about_click(int button, int state, int x, int y);

void about_keyboard(unsigned char key, int x, int y);

#endif
