/*
 * Komposter
 *
 * Copyright (c) 2010 Noora Halme et al. (see AUTHORS)
 *
 * This code is licensed under the GNU General Public
 * License version 2. See LICENSE for full text.
 *
 * Handling the configuration file ~/.komposter
 *
 */

#ifndef __DOTFILE_H__
#define __DOTFILE_H__

int dotfile_load();
int dotfile_save();
char *dotfile_getvalue(char *key);
int dotfile_setvalue(char *key, char *value);

#endif
