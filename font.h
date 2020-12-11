/*
 * Komposter
 *
 * Copyright (c) 2010 Noora Halme et al. (see AUTHORS)
 *
 * This code is licensed under the GNU General Public
 * License version 2. See LICENSE for full text.
 *
 * Font loading, pre-rendering and drawing to display
 *
 */

#ifndef __FONT_H__
#define __FONT_H__

#include "arch.h"
#include <ft2build.h>
#include FT_FREETYPE_H

int font_init(void);
void render_text(const char *text, float x, float y, int fontnr, u32 color, int align);

#endif
