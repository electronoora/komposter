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

#include "about.h"

void about_draw(void)
{
  draw_textbox((DS_WIDTH/2), (DS_HEIGHT/2), 120, 260, "", 0);
  render_text("komposter", (DS_WIDTH/2)-120, (DS_HEIGHT/2)-32, 0, 0xffb05500, 0);
  render_text(K_VERSION, (DS_WIDTH/2)+120, (DS_HEIGHT/2)-32, 2, 0xffb05500, 2);

  glBegin(GL_LINES); glColor4f(0.5f, 0.5f, 0.5f, 0.5f);
  glVertex2f((DS_WIDTH/2)-120, (DS_HEIGHT/2)-23.5);
  glVertex2f((DS_WIDTH/2)+120, (DS_HEIGHT/2)-23.5);
  glEnd();

  render_text(K_ABOUT1, (DS_WIDTH/2), (DS_HEIGHT/2)+6, 2, 0xff707070, 1);
  render_text(K_ABOUT2, (DS_WIDTH/2), (DS_HEIGHT/2)+18, 2, 0xff707070, 1);
  render_text(K_COPYRIGHT, (DS_WIDTH/2), (DS_HEIGHT/2)+50, 2, 0xff505050, 1);
}

void about_hover(int x, int y)
{
  // nothing here
}

void about_click(int button, int state, int x, int y)
{
  if (state==GLUT_DOWN) { dialog_close(); return; }
}


void about_keyboard(unsigned char key, int x, int y)
{
  if (key==27 || key==' ' || key==13) { glutIgnoreKeyRepeat(1); dialog_close(); return; }
}