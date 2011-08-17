/*
 * Komposter (c) 2010-2011 Jani Halme (Firehawk/TDA)
 *
 * This code is licensed under the GNU General Public
 * License version 2. See file 'doc/LICENSE'.
 *
 * Framework for file selector dialogs
 *
 * $Rev$
 * $Date$
 */

#ifndef __FILEDIALOG_H_
#define __FILEDIALOG_H_

#include <glob.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <sys/stat.h>

#include "arch.h"
#include "constants.h"
#include "font.h"
#include "widgets.h"

#define FDUI_FILENAME	0
#define FDUI_OK		1
#define FDUI_VSLIDER	2

#define FDEXIT_CANCEL	1
#define FDEXIT_OK	2


typedef struct {
  char title[255];
  char cpath[255];
  char fname[255];
  char fmask[255];
  char fullpath[512];

  glob_t g;
  int exitstate;
  int owconfirm;

  int hover[4];
  int listhover;
  int kbfocus;

  int sliderpos;
  int sliderstep;
  int sliderdrag;
  int slider_yofs;
  int slider_dragstart;
} filedialog;


void filedialog_open(filedialog *fd, char *ext, char *path);
void filedialog_scanpath(filedialog *fd);
void filedialog_draw(filedialog *fd);

void filedialog_hover(filedialog *fd, int x, int y);
void filedialog_click(filedialog *fd, int button, int state, int x, int y);
void filedialog_keyboard(filedialog *fd, int key);
void filedialog_drag(filedialog *fd, int x, int y);

#endif
