/*
 * Komposter (c) 2010-2011 Jani Halme (Firehawk/TDA)
 *
 * This code is licensed under the GNU General Public
 * License version 2. See file 'doc/LICENSE'.
 *
 * Info dialog                   
 *
 * $Rev$
 * $Date$
 */

#ifndef __ABOUT_H__
#define __ABOUT_H__

#include "arch.h"
#include "dialog.h"
#include "widgets.h"

void about_draw(void);
void about_hover(int x, int y);
void about_click(int button, int state, int x, int y);

#endif
