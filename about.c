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

#include "about.h"

void about_draw(void)
{
  draw_textbox((DS_WIDTH/2), (DS_HEIGHT/2), 120, 250, "", 0);
  render_text("komposter.", (DS_WIDTH/2)-112, (DS_HEIGHT/2)-32, 0, 0xffb05500, 0);
  render_text(K_VERSION, (DS_WIDTH/2)+112, (DS_HEIGHT/2)-32, 2, 0xffb05500, 2);
  glBegin(GL_LINES); glColor4f(0.5f, 0.5f, 0.5f, 0.5f);
  glVertex2f((DS_WIDTH/2)-112, (DS_HEIGHT/2)-23.5);
  glVertex2f((DS_WIDTH/2)+112, (DS_HEIGHT/2)-23.5);
  glEnd();
  render_text(K_ABOUT, (DS_WIDTH/2)-112, (DS_HEIGHT/2)-10, 2, 0xff707070, 0);
  render_text(K_COPYRIGHT, (DS_WIDTH/2)-112, (DS_HEIGHT/2)+50, 2, 0xff505050, 0);
}

void about_hover(int x, int y)
{
  // nothing here
}

void about_click(int button, int state, int x, int y)
{
  if (state==GLUT_UP && hovertest_box(x,y,(DS_WIDTH/2),(DS_HEIGHT/2),120,250)) {
    dialog_close(); return;
  }
}

