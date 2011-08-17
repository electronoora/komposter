/*
 * Komposter (c) 2010-2011 Jani Halme (Firehawk/TDA)
 *
 * This code is licensed under the GNU General Public
 * License version 2. See file 'doc/LICENSE'.
 *
 * Pattern editor page
 *
 * $Rev$
 * $Date$
 */

#include "pattern.h"

// button indexes
#define B_PREV 0
#define B_NEXT 1
#define B_SHORTER 2
#define B_LONGER 3
#define B_OCTUP 4
#define B_OCTDN 5
#define B_PREVSYN 6
#define B_SYNNAME 7
#define B_NEXTSYN 8
#define B_PATTPLAY 9
#define B_PATTLOAD 10
#define B_PATTSAVE 11
#define B_PATTCLEAR 12

#define KB_WIDTH 40

#define PIANOROLL_Y 528.5
#define PIANOROLL_X KB_WIDTH+5.5

#define PIANOROLL_HEIGHT 336

#define PIANOROLL_CELLWIDTH 11

#define PIANOROLL_CELLHEIGHT 7
#define PIANOROLL_BEAT (PIANOROLL_CELL*4)

#define PIANOROLL_OCTAVES 6


// from modules.c
extern float pitch[MAX_SYNTH];
extern int accent[MAX_SYNTH];
extern int gate[MAX_SYNTH];

extern int audiomode; // from audio.c
extern unsigned long playpos;

extern int bpm; // from sequencer.c

extern int csynth; // from synthesizer.c
extern char synthname[MAX_SYNTH][128]; // from synthesizer.c
extern char patchname[MAX_SYNTH][MAX_PATCHES][128]; // from patch.c

extern synthmodule mod[MAX_SYNTH][MAX_MODULES]; // ditto
extern int signalfifo[MAX_SYNTH][MAX_MODULES]; // module execution stack
extern int cpatch[MAX_SYNTH]; // selected patch for each synth


// from patch.c
extern char pianokeys[29];
extern int kpkeydown;



unsigned long pattdata[MAX_PATTERN][MAX_PATTLENGTH];
unsigned int pattlen[MAX_PATTERN];

int cpatt, coct;

int patt_ui[13];
int piano_hover, piano_note, piano_drag, piano_dragto;
int piano_start;
int slide_hover;
float slide_drag_xofs;
int slide_drag, slide_drag_start;
int patt_cpatch;


// initialize pattern data to defaults
void pattern_init()
{
  int i,j;
  
  cpatt=0;
  coct=3;  
  for(i=0;i<MAX_PATTERN;i++) {
    pattlen[i]=4;
    for(j=0;j<MAX_PATTLENGTH;j++) pattdata[i][j]=0;
  }
  piano_note=-1; piano_hover=-1; piano_drag=-1; piano_dragto=-1;
  piano_start=0; slide_hover=0;
  patt_cpatch=0;
  for(i=0;i<13;i++) patt_ui[i]=0;
}


void pattern_toggleplayback()
{
  int patt_playing;

  patt_playing=((audiomode==AUDIOMODE_PATTERNPLAY)&1)^1;
  patt_ui[B_PATTPLAY]&=1;
  patt_ui[B_PATTPLAY]|=(patt_playing<<1);
  playpos=0;
  gate[0]=0;
  if (patt_playing) { audiomode=AUDIOMODE_PATTERNPLAY; } else { audiomode=AUDIOMODE_COMPOSING; }
}



// returns pattern offset the cursor is on top of or -1 if the cursor is currently
// off the piano roll. when a position is returned, the int pointed by note is set to
// the midi note number under the cursor.
int pattern_cursorpos(int x, int y, int *note)
{
  int cx,cy;

  if (
      x >  PIANOROLL_X  &&
      x < (PIANOROLL_X + (pattlen[cpatt]*16)*PIANOROLL_CELLWIDTH + 1) &&
      y < PIANOROLL_Y  + 3 &&
      y > (PIANOROLL_Y + 1 - (PIANOROLL_OCTAVES*12)*PIANOROLL_CELLHEIGHT)
     )
  {
    cx=(int)((x-PIANOROLL_X)/PIANOROLL_CELLWIDTH)-1;
    cy=abs((int)((y-PIANOROLL_Y-2)/PIANOROLL_CELLHEIGHT));
    *note=coct*12+cy;
    return cx;
  }
  return -1;
}


void pattern_mouse_hover(int x, int y)
{
  int m;

  // test buttons
  patt_ui[B_PREV]=hovertest_box(x, y, 14,  DS_HEIGHT-14, 16, 16);
  patt_ui[B_NEXT]=hovertest_box(x, y, 64,  DS_HEIGHT-14, 16, 16);
  patt_ui[B_SHORTER]=hovertest_box(x, y, 98,  DS_HEIGHT-14, 16, 16);
  patt_ui[B_LONGER]=hovertest_box(x, y, 180,  DS_HEIGHT-14, 16, 16);
  patt_ui[B_OCTDN]=hovertest_box(x, y, 12, PIANOROLL_Y+16, 16, 16); 
  patt_ui[B_OCTUP]=hovertest_box(x, y, 32, PIANOROLL_Y+16, 16, 16);
  patt_ui[B_PREVSYN]=hovertest_box(x, y, 214, DS_HEIGHT-14, 16, 16);
  patt_ui[B_NEXTSYN]=hovertest_box(x, y, 430, DS_HEIGHT-14, 16, 16);

  patt_ui[B_PATTCLEAR]=hovertest_box(x, y, 555, DS_HEIGHT-14, 16, 16) | (patt_ui[B_PATTCLEAR]&8);
/*
  patt_ui[B_PATTSAVE]=hovertest_box(x, y, 585, DS_HEIGHT-14, 16, 16);
*/
  patt_ui[B_PATTPLAY]=hovertest_box(x, y, 482, DS_HEIGHT-14, 16, 42);
  if (audiomode==AUDIOMODE_PATTERNPLAY) patt_ui[B_PATTPLAY]|=2;

  // hovering above piano roll?
  piano_note=-1;
  piano_hover=pattern_cursorpos(x,y,&piano_note);

  // hovering above slider?
  slide_hover=hovertest_hslider(x,y,PIANOROLL_X, PIANOROLL_Y+12, (DS_WIDTH-(PIANOROLL_X+6)), 12,
   piano_start, (DS_WIDTH-(PIANOROLL_X+4))/PIANOROLL_CELLWIDTH, pattlen[cpatt]*16);

  // any ui-elements active?
  //for(m=0;m<7;m++) if (patt_ui[m]&1) return;
}


void pattern_mouse_drag(int x, int y)
{
  int i,m,note;
  float f, cos, cip, sbw, slw;

  // dragging a new note, paint all cells from drag point to current cell
  if (piano_drag >= 0) {
    m=pattern_cursorpos(x, y, &note); // disregard note
    if (m>=piano_drag) piano_dragto=m;
    return;
  } 

  // dragging the piano roll scrollbar
  if (slide_drag>0) {
    sbw=DS_WIDTH-(PIANOROLL_X+6); // scrollbar width
    cos=(DS_WIDTH-(PIANOROLL_X))/PIANOROLL_CELLWIDTH; // cells on screen
    cip=pattlen[cpatt]*16; // cells in pattern
    slw=sbw*(cos/cip);
    f=((x-slide_drag_xofs)/(sbw-slw))*(cip-cos);
    piano_start=slide_drag_start+f;
    if (piano_start>(1+cip-cos)) piano_start=(1+cip-cos);
    if (piano_start<0) piano_start=0;
    return;
  }
}


void pattern_mouse_click(int button, int state, int x, int y)
{
  int i,m,note,j,mi,mt;
  int patt_playing;
  float freq;
  char tmps[256];

  if (button==GLUT_LEFT_BUTTON) {
    if (state==GLUT_DOWN) {

      // test first if modifiers are pressed
      m=glutGetModifiers();
      if (m==GLUT_ACTIVE_SHIFT) {
        if (piano_hover>=0 && piano_note==(pattdata[cpatt][piano_start+piano_hover]&0xff)) {
          // wipe the note from begin to end
          i=piano_start+piano_hover;
          while(pattdata[cpatt][i]&NOTE_LEGATO) i--;
          do { pattdata[cpatt][i++]=0;
          } while(pattdata[cpatt][i]&NOTE_LEGATO);
        }
        return;
      }

      // clear the pattern
      if (patt_ui[B_PATTCLEAR]&1) {
        if (patt_ui[B_PATTCLEAR]&8) {
          for(i=0;i<MAX_PATTLENGTH;i++) pattdata[cpatt][i]=0;
          sprintf(tmps, "Pattern %02d cleared", cpatt);
          console_post(tmps);
          patt_ui[B_PATTCLEAR]&=0xff-8;
        } else {
          console_post("Click again to clear pattern");
          patt_ui[B_PATTCLEAR]|=8;
        }
        return;
      } else {
        patt_ui[B_PATTCLEAR]&=0xff-8;
      }

      // click on the ui buttons
      if (patt_ui[B_PREV]) { if (cpatt>0) cpatt--; return; }
      if (patt_ui[B_NEXT]) { if (cpatt<MAX_PATTERN) cpatt++; return; }
      if (patt_ui[B_SHORTER]) {
        if (pattlen[cpatt]>1) pattlen[cpatt]/=2;
        while (piano_start > 
          ( 1+pattlen[cpatt]*16 - ( (DS_WIDTH-(PIANOROLL_X))/PIANOROLL_CELLWIDTH ) ) ) piano_start--;
        if (piano_start<0) piano_start=0;
        return; }
      if (patt_ui[B_LONGER]) { if (pattlen[cpatt]<16) pattlen[cpatt]*=2; return; }
      if (patt_ui[B_OCTDN]) { if (coct>0) coct--; return; }
      if (patt_ui[B_OCTUP]) { if (coct<(9-PIANOROLL_OCTAVES)) coct++; return; }
      if (patt_ui[B_PREVSYN]) { if (csynth[cpatch]>0) csynth[cpatch]--; 
        audio_loadpatch(0, csynth, cpatch[csynth]);        
        return; }
      if (patt_ui[B_NEXTSYN]) { if (csynth[cpatch]<(MAX_PATCHES-1)) csynth[cpatch]++; 
        audio_loadpatch(0, csynth, cpatch[csynth]);
        return; }

      if (patt_ui[B_PATTPLAY]&1) pattern_toggleplayback();
      
      // click on the piano roll?
      if (piano_hover>=0) {
        // yes - set the 1/16th note on the pattern
        pattdata[cpatt][piano_start+piano_hover]=piano_note;
        piano_drag=piano_hover; //drag note stating from this cell
        piano_dragto=piano_hover;
        
        // trig the note to get an audible feedback
        audio_trignote(0, piano_note);
      }
      
      // click on the slider?
      if (slide_hover) {
        slide_drag=1;
        slide_drag_xofs=x;
        slide_drag_start=piano_start;
      }
    }
    if (state==GLUT_UP) {
      if (slide_drag) { slide_drag=0; return; }
      if (piano_drag>=0) {
        // released the drag, draw the notes with legato and reset drag
        for(m=piano_drag;m<=piano_dragto;m++) pattdata[cpatt][piano_start+m]=pattdata[cpatt][piano_start+piano_drag]|NOTE_LEGATO;
        pattdata[cpatt][piano_start+piano_drag]&=0xff; // legato off from the first note
        pattdata[cpatt][piano_start+piano_dragto+1]&=0xff; // legato off from the note which follows
        piano_drag=-1;
        piano_dragto=-1;

        gate[0]=0; // note off
      }
    } 
  }            

  
  if (button==GLUT_RIGHT_BUTTON) {
    if (state==GLUT_DOWN) {
    
      if (piano_hover>=0) {
        // right click to set accent
        if (pattdata[cpatt][piano_start+piano_hover]) { // got a note here
          if (pattdata[cpatt][piano_start+piano_hover]&NOTE_LEGATO) {
            //printf("legato here\n"); // do nothing
          } else {
            // first note, so mark it with an accent
            pattdata[cpatt][piano_start+piano_hover]^=NOTE_ACCENT;
          }
        }
      }
      
      
    }
  }
}


// keyboard callback
void pattern_keyboard(unsigned char key, int x, int y)
{
  int i,j,m,pkey;
  int mi, mt;
  float freq;

  switch (key) {
    case ' ': pattern_toggleplayback(); return; break;
    case '-':
      if (pattlen[cpatt]>1) pattlen[cpatt]/=2;
      while (piano_start > 
        ( 1+pattlen[cpatt]*16 - ( (DS_WIDTH-(PIANOROLL_X))/PIANOROLL_CELLWIDTH ) ) ) piano_start--;
      if (piano_start<0) piano_start=0;
      return;
      break;
    case '+':
      if (pattlen[cpatt]<16) pattlen[cpatt]*=2;
      return;
      break;
    case ',':
      if (csynth[cpatch]>0) csynth[cpatch]--; 
      audio_loadpatch(0, csynth, cpatch[csynth]);  
      return; 
      break;
    case '.':
      if (csynth[cpatch]<(MAX_PATCHES-1)) csynth[cpatch]++; 
      audio_loadpatch(0, csynth, cpatch[csynth]);
      return;
      break;
  }

  // keyjazzing! w00t!
  for (i=0;i<29;i++) {
    if (pianokeys[i]==key) {
      // trig note. use same base octave as pattern editor
      pkey=coct*12 + i;
      kpkeydown=pkey;
      audio_trignote(0, pkey);      
    }
  }
}


void pattern_specialkey(unsigned char key, int x, int y)
{
  switch(key)
  {
    case GLUT_KEY_LEFT:
      if (cpatt>0) cpatt--;
      break;
      
    case GLUT_KEY_RIGHT:
      if (cpatt<MAX_PATTERN) cpatt++;
      break;

    case GLUT_KEY_DOWN:
      if (coct>0) coct--;
      break;
      
    case GLUT_KEY_UP:
      if (coct<(9-PIANOROLL_OCTAVES)) coct++;
      break;

    case GLUT_KEY_PAGE_DOWN:
      break;

    case GLUT_KEY_PAGE_UP:
      break;
  }
}





void pattern_keyboardup(unsigned char key, int x, int y)
{
  int i;

  for (i=0;i<29;i++) {
    if (pianokeys[i]==key) {
      // if this key is still down, drop gate and trig
      if (kpkeydown==(i+coct*12)) {
        gate[0]=0;
        kpkeydown=-1;
      }
    }
  }
}





void pattern_draw(void)
{
  int i,j,n,l;
  char tmps[256];
  long ticks;
  int rkdown;

  // draw keyboard, highlight a note if cursor is on piano roll
  rkdown=-1;
  if (piano_drag>=0) rkdown=piano_drag;
  if (kpkeydown>=0) rkdown=kpkeydown;  
  for(i=0;i<PIANOROLL_OCTAVES;i++)
    draw_kboct(round(PIANOROLL_Y-(12*PIANOROLL_CELLHEIGHT*i)), 40, 12, coct+i, piano_note, rkdown);   

  // divider lines for notes
  for(j=0;j<(PIANOROLL_OCTAVES*12+1);j++) {
    glColor4f(0.3, 0.3, 0.3, 0.8);
    if ((j%12)==0)  glColor4f(0.5, 0.5, 0.5, 0.8);
    glEnable(GL_LINE_STIPPLE);
    glLineStipple(1, 0xaaaa);
    glBegin(GL_LINES);
    glVertex2f(PIANOROLL_X,     PIANOROLL_Y-(j*PIANOROLL_CELLHEIGHT));
    glVertex2f(PIANOROLL_X+(pattlen[cpatt]*16-piano_start)*PIANOROLL_CELLWIDTH, PIANOROLL_Y-(j*PIANOROLL_CELLHEIGHT));
    glEnd();
    glDisable(GL_LINE_STIPPLE);
  }  
  
  // draw markers for beats and 1/16th notes
  for(i=0,j=piano_start;j<(pattlen[cpatt]*16+1);i++,j++) {
    glColor4f(0.2, 0.2, 0.2, 0.8);
    if ((j%4)==0) {
      // beat
      glColor4f(0.5, 0.5, 0.5, 0.8);
    }
    if ((j%16)==0 && j<(pattlen[cpatt]*16)) {
      // measure
      sprintf(tmps, "%1d", (j/16));
      render_text(tmps, 2.5+PIANOROLL_X+(i*PIANOROLL_CELLWIDTH), round(PIANOROLL_Y+PIANOROLL_CELLHEIGHT), 2, 0xffa0a0a0, 0); 
      glColor4f(0.7, 0.7, 0.7, 0.8);
    }
    glEnable(GL_LINE_STIPPLE);
    glLineStipple(1, 0xaaaa);
    glBegin(GL_LINES);
    glVertex2f(PIANOROLL_X+i*PIANOROLL_CELLWIDTH, round(PIANOROLL_Y+PIANOROLL_CELLHEIGHT));
    glVertex2f(PIANOROLL_X+i*PIANOROLL_CELLWIDTH, round(PIANOROLL_Y-((PIANOROLL_OCTAVES*12)*PIANOROLL_CELLHEIGHT)));
    glEnd();
    glDisable(GL_LINE_STIPPLE);
  }

  // draw the playing position if pattern is playing
  if (audiomode==AUDIOMODE_PATTERNPLAY) {
    ticks=playpos / (OUTPUTFREQ/(bpm*256/60)); // calc tick from sample index
    if ( (ticks>>6)>=piano_start && (ticks>>6)<(pattlen[cpatt]*16) ) // TODO: test if pos > pianostart+rollwidth
    {
      // within bounds
      i=(ticks>>6)-piano_start;
      glColor4f(1.0, 1.0, 1.0, 1.0);
      glBegin(GL_LINES);
      glVertex2f(PIANOROLL_X+i*PIANOROLL_CELLWIDTH, round(PIANOROLL_Y+PIANOROLL_CELLHEIGHT));
      glVertex2f(PIANOROLL_X+i*PIANOROLL_CELLWIDTH, round(PIANOROLL_Y-((PIANOROLL_OCTAVES*12)*PIANOROLL_CELLHEIGHT)));
      glEnd();
    }
  }
  
  // draw the notes to the piano roll
  for(i=0,j=piano_start;j<(pattlen[cpatt]*16);i++,j++) {
    n=pattdata[cpatt][j] & 0xff; // take only the low byte
    l=(pattdata[cpatt][j]&NOTE_LEGATO) ? 0 : 1; // if legato, extend note for a pixel left
    if (n>0) { // todo: fill with -1 instead as 0 is a valid midi note
      if ( n>=(coct*12) && n<((coct+PIANOROLL_OCTAVES)*12)) {
        // on the visible three octaves - plot it
        glColor4ub(0xb0, 0x55, 0x00, 0xff);
        glBegin(GL_QUADS);
          glVertex2f(l+PIANOROLL_X+i*PIANOROLL_CELLWIDTH, PIANOROLL_Y-1-(n-coct*12)*PIANOROLL_CELLHEIGHT);
          glVertex2f(PIANOROLL_X+(i+1)*PIANOROLL_CELLWIDTH, PIANOROLL_Y-1-(n-coct*12)*PIANOROLL_CELLHEIGHT);          
          glVertex2f(PIANOROLL_X+(i+1)*PIANOROLL_CELLWIDTH, PIANOROLL_Y-(n-coct*12+1)*PIANOROLL_CELLHEIGHT);
          glVertex2f(l+PIANOROLL_X+i*PIANOROLL_CELLWIDTH, PIANOROLL_Y-(n-coct*12+1)*PIANOROLL_CELLHEIGHT);
        glEnd();
        if (pattdata[cpatt][j]&NOTE_ACCENT) {
          // draw the accent mark
          render_text(">", 
            l+PIANOROLL_X+i*PIANOROLL_CELLWIDTH+1, 
            PIANOROLL_Y-2-(n-coct*12+1)*PIANOROLL_CELLHEIGHT,
            2, 0x000000ff, 0);
          render_text(">", 
            l+PIANOROLL_X+i*PIANOROLL_CELLWIDTH, 
            PIANOROLL_Y-2-(n-coct*12+1)*PIANOROLL_CELLHEIGHT,
            2, 0xffffffff, 0);
        }
      }
    }
  }

  // draw the note being dragged
  if (piano_drag >= 0) {
    n=pattdata[cpatt][piano_start+piano_drag];
        glColor4ub(0xc0, 0xc0, 0xc0, 0x6f);
        glBegin(GL_QUADS);
          glVertex2f(1+PIANOROLL_X+piano_drag*PIANOROLL_CELLWIDTH, PIANOROLL_Y-1-(n-coct*12)*PIANOROLL_CELLHEIGHT);
          glVertex2f(PIANOROLL_X+(piano_dragto+1)*PIANOROLL_CELLWIDTH, PIANOROLL_Y-1-(n-coct*12)*PIANOROLL_CELLHEIGHT);          
          glVertex2f(PIANOROLL_X+(piano_dragto+1)*PIANOROLL_CELLWIDTH, PIANOROLL_Y-(n-coct*12+1)*PIANOROLL_CELLHEIGHT);
          glVertex2f(1+PIANOROLL_X+piano_drag*PIANOROLL_CELLWIDTH, PIANOROLL_Y-(n-coct*12+1)*PIANOROLL_CELLHEIGHT);
        glEnd();    
  }

  // highlight the note row if a key is currently down
  if (kpkeydown >=0 && kpkeydown>=(coct*12) && kpkeydown<((coct+PIANOROLL_OCTAVES)*12)) {
    glColor4ub(0xc0, 0xc0, 0xc0, 0x3f);   
    glBegin(GL_QUADS);

          glVertex2f(PIANOROLL_X, 
                     PIANOROLL_Y-1-(kpkeydown-coct*12)*PIANOROLL_CELLHEIGHT);
                     
          glVertex2f(PIANOROLL_X+(pattlen[cpatt]*16-piano_start)*PIANOROLL_CELLWIDTH, 
                     PIANOROLL_Y-1-(kpkeydown-coct*12)*PIANOROLL_CELLHEIGHT);          
                     
          glVertex2f(PIANOROLL_X+(pattlen[cpatt]*16-piano_start)*PIANOROLL_CELLWIDTH, 
                     PIANOROLL_Y-(kpkeydown-coct*12+1)*PIANOROLL_CELLHEIGHT);
                     
          glVertex2f(PIANOROLL_X, 
                     PIANOROLL_Y-(kpkeydown-coct*12+1)*PIANOROLL_CELLHEIGHT);
    
    glEnd();        
  }

  // draw the horizontal slider
  draw_hslider(PIANOROLL_X, PIANOROLL_Y+12, (DS_WIDTH-(PIANOROLL_X+6)), 12,
   piano_start, (DS_WIDTH-(PIANOROLL_X+4))/PIANOROLL_CELLWIDTH, 
   pattlen[cpatt]*16,
   slide_hover);

  // draw the ui elements on the pattern page
  draw_button(14, DS_HEIGHT-14, 16, "<<", patt_ui[B_PREV]);
  sprintf(tmps, "%02d", cpatt);
  draw_textbox(39, DS_HEIGHT-14, 16, 24, tmps, 0);
  draw_button(64, DS_HEIGHT-14, 16, ">>", patt_ui[B_NEXT]);

  draw_button(98, DS_HEIGHT-14, 16, "<<", patt_ui[B_SHORTER]);
  sprintf(tmps, "%1d meas", pattlen[cpatt]);
  draw_textbox(139, DS_HEIGHT-14, 16, 56, tmps, 0);
  draw_button(180, DS_HEIGHT-14, 16, ">>", patt_ui[B_LONGER]);            

  draw_button(12, PIANOROLL_Y+15.5, 16, "DN", patt_ui[B_OCTDN]);
  draw_button(32, PIANOROLL_Y+15.5, 16, "UP", patt_ui[B_OCTUP]);

  draw_button(214, DS_HEIGHT-14, 16, "<<", patt_ui[B_PREVSYN]);
  sprintf(tmps, "%02d:%s", csynth[cpatch], patchname[csynth][csynth[cpatch]]);
  draw_textbox(322, DS_HEIGHT-14, 16, 188, tmps, patt_ui[B_SYNNAME]);
  draw_button(430, DS_HEIGHT-14, 16, ">>", patt_ui[B_NEXTSYN]);

  ticks=playpos / (OUTPUTFREQ/(bpm*256/60));
  if (audiomode==AUDIOMODE_PATTERNPLAY) {
    sprintf(tmps, "%02ld:%02ld", (ticks>>10), ((ticks>>6)&15));
  } else {
    sprintf(tmps, "play");
  }
  draw_textbox(482, DS_HEIGHT-14, 16, 42, tmps, patt_ui[B_PATTPLAY]);

  draw_button(555, DS_HEIGHT-14, 16, "C", patt_ui[B_PATTCLEAR]);

/*
  draw_button(585, DS_HEIGHT-14, 16, "S", patt_ui[B_PATTSAVE]);
*/
}
    