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
#include "console.h"

int logptr=0;
char backlog[BACKLOG][256];
float logdisp=0.0f;


void console_post(char *msg)
{
  logptr++; logptr=(logptr%BACKLOG);
  strncpy((char*)(&backlog[logptr]), msg, 256);
  logdisp=LOG_INITIAL_T;
  // printf("%s\n",msg);
}

char *console_latest(void)
{
  return (char*)(&backlog[logptr]);
}

void console_advanceframe(void)
{
  if (logdisp>0) logdisp-=LOG_DELTA_T;
}

void console_print(int x, int y)
{
  float a=1.0f;

  if (logdisp<=0.0f) return;
  if (logdisp<1.0f) a*=logdisp;
  a*=255;
  render_text(backlog[logptr], x+1, y+1, 2, ((unsigned char)(a)<<24)|0x00000000, 0);
  render_text(backlog[logptr], x, y, 2, ((unsigned char)(a)<<24)|0x00b05500, 0);  

}
