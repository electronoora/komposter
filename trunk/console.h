/*
 * Komposter (c) 2010-2011 Jani Halme (Firehawk/TDA)
 *
 * This code is licensed under the GNU General Public
 * License version 2. See file 'doc/LICENSE'.
 *
 * Console logging
 *
 * $Rev$
 * $Date$
 */

#ifndef __CONSOLE_H__
#define __CONSOLE_H__

#include "arch.h"
#include "font.h"

#define BACKLOG 100
#define LOG_INITIAL_T 18.0
#define LOG_DELTA_T 0.1

void console_post(char *msg);
char *console_latest(void);
void console_advanceframe(void);
void console_print(int x, int y);

#endif
