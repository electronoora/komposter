/*
 * Komposter (c) 2010-2011 Jani Halme (Firehawk/TDA)
 *
 * This code is licensed under the GNU General Public
 * License version 2. See file 'doc/LICENSE'.
 *
 * Handling the configuration file ~/.komposter
 *
 * $Rev$
 * $Date$
 */

#ifndef __DOTFILE_H__
#define __DOTFILE_H__

int dotfile_load();
int dotfile_save();
char *dotfile_getvalue(char *key);
int dotfile_setvalue(char *key, char *value);

#endif
