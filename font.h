/*
 * Komposter (c) 2010-2011 Jani Halme (Firehawk/TDA)
 *
 * This code is licensed under the GNU General Public
 * License version 2. See file 'doc/LICENSE'.
 *
 * Font loading, pre-rendering and drawing to display
 *
 * $Rev$
 * $Date$
 */

#ifndef __FONT_H__
#define __FONT_H__

#include "arch.h"
#include <ft2build.h>
#include FT_FREETYPE_H

int font_init(void);
void render_text(char *text, float x, float y, int fontnr, unsigned long color, int align);

#endif
