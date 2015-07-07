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

#include "dialog.h"

// global vars for modal dialogs
int dialog_active=0;

// function pointers to dialog actions
void (*dialog_drawfunc)(void) = NULL;
void (*dialog_hoverfunc)(int,int) = NULL;
void (*dialog_clickfunc)(int,int,int,int) = NULL;
void (*dialog_kbfunc)(unsigned char,int,int) = NULL;
void (*dialog_dragfunc)(int,int) = NULL;
void (*dialog_specialfunc)(int,int,int) = NULL;

int is_dialog(void)
{
  return dialog_active;
}

int is_dialogkb(void)
{
  return (dialog_kbfunc!=NULL);
}

int is_dialogdrag(void) {
  return (dialog_dragfunc!=NULL);
}

void dialog_open(void *draw, void *hover, void *click)
{
  dialog_active=1;
  dialog_drawfunc=draw;
  dialog_hoverfunc=hover;
  dialog_clickfunc=click;
}


void dialog_bindkeyboard(void *kbfunc)
{
  dialog_kbfunc=kbfunc;
}

void dialog_binddrag(void *dragfunc)
{
  dialog_dragfunc=dragfunc;
}

void dialog_bindspecial(void *specialfunc)
{
  dialog_specialfunc=specialfunc;
}

void dialog_close(void)
{
  dialog_active=0;
  dialog_drawfunc=NULL;
  dialog_hoverfunc=NULL;
  dialog_clickfunc=NULL;
  dialog_kbfunc=NULL;
  dialog_dragfunc=NULL;
  dialog_specialfunc=NULL;
}


void dialog_draw(void)
{
  draw_dimmer();
  dialog_drawfunc();
}

void dialog_hover(int x, int y)
{
  dialog_hoverfunc(x, y);
}

void dialog_click(int button, int state, int x, int y)
{
  if (dialog_clickfunc) dialog_clickfunc(button, state, x, y);
}

void dialog_keyboard(unsigned char key, int x, int y)
{
  if (dialog_kbfunc) dialog_kbfunc(key, x, y);
}

void dialog_drag(int x, int y)
{
  if (dialog_dragfunc) dialog_dragfunc(x, y);
}

void dialog_special(int key, int x, int y)
{
  if (dialog_specialfunc) dialog_specialfunc(key, x, y);
}
