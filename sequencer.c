/*
 * Komposter (c) 2010-2011 Jani Halme (Firehawk/TDA)
 *
 * This code is licensed under the GNU General Public
 * License version 2. See file 'doc/LICENSE'.
 *
 * Sequencer page
 *
 * $Rev$
 * $Date$
 */

#include "sequencer.h"

// button indexes
#define B_DECCH 	0
#define B_ADDCH 	1
#define B_BPMDN 	2
#define B_BPMUP 	3
#define B_SSHORTER 	4
#define B_SLONGER 	5

#define B_PATT_ADDNUM	6
#define B_PATT_DECNUM	7
#define B_PATT_ADDREP	8
#define B_PATT_DECREP	9
#define B_PATT_ADDTRANS 10
#define B_PATT_DECTRANS 11
#define B_PATT_ADD	12
#define B_PATT_PREVPATCH	13
#define B_PATT_NEXTPATCH	14

#define B_CHAN_PREVSYNTH	15
#define B_CHAN_NEXTSYNTH	16

#define B_SEQPLAY		17
#define B_REWIND		18
#define B_CLEAR			19

#define B_PREVIEW_REWIND	20
#define B_PREVIEW_PLAY		21
#define B_PREVIEW_EXPORT	22

#define	B_SAVE_SONG		23
#define B_LOAD_SONG		24

#define B_ENVRESTART		25
#define B_VCORESTART		26
#define B_LFORESTART		27

#define B_NEWSONG		28
#define B_RENDER		29

#define B_LOOP			30


#define SEQUENCER_Y 14.5
#define SEQUENCER_X 45.5
#define SEQUENCER_CELLWIDTH 14
#define SEQUENCER_CELLHEIGHT 14

// file dialogs
#define FD_SAVE 0
#define FD_LOAD 1

// file dialog structs for load/save synth
filedialog songfd[2];
int songfd_active=-1;




int seqch;
int seqsonglen;
int seq_start;
int bpm;
int seq_synth[MAX_CHANNELS]; // which synth assigned to each channel
int seq_restart[MAX_CHANNELS]; // channel restart flags

int seq_hover_ch;
int seq_hover_meas;
int seq_chlabel_hover;
int seq_timeline_hover;

int seq_render_start;
int seq_render_end;
int seq_render_drag;

int seq_render_hover;

int seqslide_hover;
int seqslide_drag;
int seqslide_drag_xofs;
int seqslide_drag_start;

int seq_drag_active; // are we dragging currently?
int seq_drag_grabpos; // the measure from which the pattern was grabbed
int seq_drag_pattstart; // the first measure of the pattern being dragged
int seq_drag_droppos; // where the pattern is currently dragged to (draw ghost here)
int seq_drag_pattch;

int seq_ui[31];

int seq_add_patt;
int seq_add_repeat;
int seq_add_transpose;
int seq_add_editmode;
int seq_add_patch;

// sequencer data
int seq_pattern[MAX_CHANNELS][MAX_SONGLEN];
int seq_repeat[MAX_CHANNELS][MAX_SONGLEN];
int seq_transpose[MAX_CHANNELS][MAX_SONGLEN];
int seq_patch[MAX_CHANNELS][MAX_SONGLEN];


// from main
extern int cpage;

// from modules.c
extern int gate[MAX_SYNTH];

// from pattern.c
extern unsigned int pattlen[MAX_PATTERN];
extern int cpatch[MAX_SYNTH];
extern int cpatt;

// from synthesizer.c
extern char synthname[MAX_SYNTH][128];
extern int csynth;

// from patch.c
extern char patchname[MAX_SYNTH][MAX_PATCHES][128];

// from audio.c
extern int audiomode;
extern int render_state;
extern long render_bufferlen;
extern long render_pos;
extern short *render_buffer;
extern long render_playpos;
extern int render_type;
extern float audio_peak;
extern float audio_latest_peak;
extern int render_live_loop;


// initialize seq data to defaults
void sequencer_init()
{
  int i,j;
  
  seqch=4;
  seqsonglen=128;
  bpm=125;
  seq_start=0;

  seq_hover_ch=-1;
  seq_hover_meas=-1;
  seq_chlabel_hover=-1;
  seq_timeline_hover=-1;

  seq_render_start=-1;
  seq_render_end=-1;
  seq_render_hover=-1;
  seq_render_drag=0;

  seqslide_hover=0;
  seqslide_drag=0;
  seqslide_drag_xofs=0;
  seqslide_drag_start=0;

  seq_add_patt=0;
  seq_add_repeat=1;
  seq_add_transpose=0;
  seq_add_editmode=0;

  for(i=0;i<30;i++) seq_ui[i]=0;
  for(i=0;i<MAX_CHANNELS;i++) { seq_synth[i]=0; seq_restart[i]=0; }
  for(i=0;i<MAX_CHANNELS;i++)
    for(j=0;j<MAX_SONGLEN;j++) {
      seq_pattern[i][j]=-1;
      seq_repeat[i][j]=0;
      seq_transpose[i][j]=0;
      seq_patch[i][j]=0;
    }

  // no dialogs visible
  songfd_active=-1;
  strcpy((char*)(&songfd[FD_SAVE].title), "save song");
  strcpy((char*)(&songfd[FD_LOAD].title), "load song");  
}


void sequencer_clearsong(void)
{
  int i,j;

  seqch=4;
  seqsonglen=128;
  bpm=125;
  seq_start=0;

  seq_render_start=-1;
  seq_render_end=-1;

  for(i=0;i<MAX_CHANNELS;i++) { seq_synth[i]=0; seq_restart[i]=0; }
  for(i=0;i<MAX_CHANNELS;i++)
    for(j=0;j<MAX_SONGLEN;j++) {
      seq_pattern[i][j]=-1;
      seq_repeat[i][j]=0;
      seq_transpose[i][j]=0;
      seq_patch[i][j]=0;
    }
}


void sequencer_toggleplayback() {
  int seq_playing;

  seq_playing=((audiomode==AUDIOMODE_PLAY)&1)^1;
  seq_ui[B_SEQPLAY]&=1;
  seq_ui[B_SEQPLAY]|=(seq_playing<<1);
  // TODO: reset all synths every time play starts/stops
  if (seq_playing) {
    audiomode=AUDIOMODE_PLAY;
  } else {
    audio_panic();
    audiomode=AUDIOMODE_COMPOSING;
  }
}



// scan the sequencer pattern list and see if there is a pattern which spans to the
// grid position clicked
int sequencer_ispattern(int ch, int clickpos)
{
  int i;

  for(i=clickpos;i>=0;i--) {
    if (seq_pattern[ch][i]>=0) {
      if (clickpos >= i && clickpos < (i+pattlen[seq_pattern[ch][i]]*seq_repeat[ch][i])) {
        // the pattern spans to the clicked position, return true
        return 1;
      }
    }
  }
  return 0;
}

// starting position of the pattern which spans to clicked position
int sequencer_patternstart(int ch, int clickpos)
{
  int i;

  for(i=clickpos;i>=0;i--) {
    if (seq_pattern[ch][i]>=0) {
      if (clickpos >= i && clickpos < (i+pattlen[seq_pattern[ch][i]]*seq_repeat[ch][i])) {
        // the pattern spans to the clicked position, return pattern start index
        return i;
      }
    }
  }
  return -1; // nothing here
}





// returns currently pointed channel and measure into integers pointed
// by the caller. both are set to -1 if cursor is outside the sequencer
// grid. returns 0 if off-grid, 1 if on audio channels, 2 if on modulator
// channel (=255)
int sequencer_cursorpos(int x, int y, int *channel, int *measure)
{
  int cx,cy;

  if (
      x >  SEQUENCER_X  &&
      x < (SEQUENCER_X + seqsonglen*SEQUENCER_CELLWIDTH + 1) &&
      y >  SEQUENCER_Y  &&
      y < (SEQUENCER_Y + seqch*SEQUENCER_CELLHEIGHT)
     )
  {
    cx=(int)((x-SEQUENCER_X)/SEQUENCER_CELLWIDTH);
    cy=(int)((y-SEQUENCER_Y)/SEQUENCER_CELLHEIGHT);
    *channel=cy; *measure=cx+seq_start;
    return 1;
  }

  // test the modulator channel
  if (
      x >  SEQUENCER_X  &&
      x < (SEQUENCER_X + seqsonglen*SEQUENCER_CELLWIDTH + 1) &&
      y > (SEQUENCER_Y + (seqch+2)*SEQUENCER_CELLHEIGHT) &&
      y < (SEQUENCER_Y + (seqch+3)*SEQUENCER_CELLHEIGHT)
     )
  {
    cx=(int)((x-SEQUENCER_X)/SEQUENCER_CELLWIDTH);
    *channel=255; *measure=cx+seq_start;
    return 2;
  }
  *channel=-1;
  *measure=-1;
  return 0;
}


int sequencer_timelinepos(int x, int y) {
  // test the modulator channel
  if (
      x >  SEQUENCER_X  &&
      x < (SEQUENCER_X + seqsonglen*SEQUENCER_CELLWIDTH + 1) &&
      y > (SEQUENCER_Y + (seqch+3)*SEQUENCER_CELLHEIGHT) &&
      y < (SEQUENCER_Y + (seqch+4)*SEQUENCER_CELLHEIGHT+4)
     )
    return (int)((x-SEQUENCER_X)/SEQUENCER_CELLWIDTH);
  return -1;
}


void sequencer_mouse_hover(int x, int y)
{
  int m;

  // test buttons
  seq_ui[B_DECCH]=hovertest_box(x, y, 14,   DS_HEIGHT-14, 16, 16);
  seq_ui[B_ADDCH]=hovertest_box(x, y, 80,   DS_HEIGHT-14, 16, 16);
  seq_ui[B_BPMDN]=hovertest_box(x, y, 114,  DS_HEIGHT-14, 16, 16);
  seq_ui[B_BPMUP]=hovertest_box(x, y, 188,  DS_HEIGHT-14, 16, 16);
  seq_ui[B_SSHORTER]=hovertest_box(x, y, 222, DS_HEIGHT-14, 16, 16);
  seq_ui[B_SLONGER]=hovertest_box(x, y, 304, DS_HEIGHT-14, 16, 16);

  seq_ui[B_SAVE_SONG]=hovertest_box(x, y, 350, DS_HEIGHT-14, 16, 16);
  seq_ui[B_LOAD_SONG]=hovertest_box(x, y, 372, DS_HEIGHT-14, 16, 16);

  seq_ui[B_CLEAR]=hovertest_box(x, y, 462, DS_HEIGHT-14, 16, 16);
  seq_ui[B_REWIND]=hovertest_box(x, y, 440, DS_HEIGHT-14, 16, 16);

  seq_ui[B_NEWSONG]=hovertest_box(x, y, 394, DS_HEIGHT-14, 16, 16) | (seq_ui[B_NEWSONG]&8);
  
  seq_ui[B_LOOP]=hovertest_box(x, y, 630, DS_HEIGHT-14, 16, 48);

  seq_ui[B_SEQPLAY]&=0xfe; seq_ui[B_RENDER]=0;
  if (seq_render_start >= 0 && seq_render_end >= 0 && seq_render_start < seq_render_end) {
    seq_ui[B_RENDER]=hovertest_box(x, y, 578, DS_HEIGHT-14, 16, 42);
  }
  if (audiomode!=AUDIOMODE_PLAY) seq_ui[B_SEQPLAY]=0;
  if (render_state==RENDER_COMPLETE) seq_ui[B_SEQPLAY]=0;
  if (seq_render_start >= 0 ) {
    seq_ui[B_SEQPLAY]|=hovertest_box(x, y, 526, DS_HEIGHT-14, 16, 42);
  }

  seqslide_hover=hovertest_hslider(x,y,SEQUENCER_X, SEQUENCER_Y+(seqch+5)*SEQUENCER_CELLHEIGHT, (DS_WIDTH-(SEQUENCER_X+6)), 12,
   seq_start, (DS_WIDTH-(SEQUENCER_X+4))/SEQUENCER_CELLWIDTH, seqsonglen);

  m=sequencer_cursorpos(x, y, &seq_hover_ch, &seq_hover_meas);

  seq_timeline_hover=sequencer_timelinepos(x, y);
  
  // hovering on channel labels?
  seq_chlabel_hover=-1;
  if (
      x > 0 && x < SEQUENCER_X &&
      y >  SEQUENCER_Y  &&
      y < (SEQUENCER_Y + (seqch+3)*SEQUENCER_CELLHEIGHT)
     )
  {
    m=(int)((y-SEQUENCER_Y)/SEQUENCER_CELLHEIGHT);
    if (m==(seqch+2)) { m=255; } else { if (m>seqch) m=-1; }
    seq_chlabel_hover=m;
  }
}


void sequencer_mouse_drag(int x, int y)
{
  float f, cos, cip, sbw, slw;
  int dragch, dragmeas;

  // dragging the piano roll scrollbar
  if (seqslide_drag>0) {
    sbw=DS_WIDTH-(SEQUENCER_X+6); // scrollbar width
    cos=(DS_WIDTH-(SEQUENCER_X))/SEQUENCER_CELLWIDTH; // cells on screen
    cip=seqsonglen; //pattlen[cpatt]*16; // cells in pattern
    slw=sbw*(cos/cip);
    f=((x-seqslide_drag_xofs)/(sbw-slw))*(cip-cos);
    seq_start=seqslide_drag_start+f;
    if (seq_start>(1+cip-cos)) seq_start=(1+cip-cos);
    if (seq_start<0) seq_start=0;
    return;
  }
  
  // if pattern drag is active, draw a "ghost" of it. it cannot be dragged out of
  // bounds, so stop it from moving beyond the edges
  if (seq_drag_active) {
    sequencer_cursorpos(x, y, &dragch, &dragmeas);
    seq_drag_droppos=dragmeas;
  }
  
  if (seq_render_drag) {
    seq_timeline_hover=sequencer_timelinepos(x, y);
    seq_render_end=seq_timeline_hover+seq_start;
  }
  
}


void sequencer_mouse_click(int button, int state, int x, int y)
{
  int i,j,pl,m,n;
  int dragdiff;
  char *songdir;
  int seq_playing;
  
  seq_playing=((audiomode==AUDIOMODE_PLAY)&1);

  if (button==GLUT_LEFT_BUTTON) {
    if (state==GLUT_DOWN) {

      // clear the song
      if (seq_ui[B_NEWSONG]&1) {
        if (seq_ui[B_NEWSONG]&8) {
          // wipe everything
          synth_lockaudio();
          for(i=0;i<MAX_SYNTH;i++) synth_clear(i);
          patch_init();
          pattern_init();
          sequencer_clearsong();
          synth_releaseaudio();
          console_post("Song cleared and everything reset back to defaults");
          seq_ui[B_NEWSONG]&=0xff-8;
        } else {
          console_post("Click again to start a new song from scratch");
          seq_ui[B_NEWSONG]|=8;
        }
        return;
      } else {
        seq_ui[B_NEWSONG]&=0xff-8;
      }

      // test ui elements      
      if (seq_ui[B_DECCH]) { if (seqch>2) seqch--; return; }
      if (seq_ui[B_ADDCH]) { if (seqch<MAX_CHANNELS) seqch++; return; }
      if (seq_ui[B_BPMDN]) { if (bpm>0) bpm--; return; }
      if (seq_ui[B_BPMUP]) { if (bpm<255) bpm++; return; }
      if (seq_ui[B_SSHORTER]) { if (seqsonglen>1) seqsonglen--; return; }
      if (seq_ui[B_SLONGER]) { if (seqsonglen<(MAX_SONGLEN-1)) seqsonglen++; return; }

      if (seq_ui[B_CLEAR]) { if (!seq_playing) { seq_render_start=0; seq_render_end=0; } return; }
      if (seq_ui[B_REWIND]) {
        if (!seq_playing) {
          for(i=0,m=0,seq_render_start=0;i<seqsonglen;i++) for(j=0;j<seqch;j++) {
            if (seq_pattern[j][i]>=0) {
              n=i+seq_repeat[j][i]*pattlen[seq_pattern[j][i]];
              if (n>m) m=n;
            }
          } 
          seq_render_end=m;
        }
        return;
      }
     
      if (seq_ui[B_RENDER] && seq_render_start >= 0 && seq_render_end >= 0 && seq_render_start < seq_render_end) {
        audiomode=AUDIOMODE_PLAY;
        render_type=RENDER_IN_PROGRESS; // pre-render first, then play
        render_state=RENDER_START;
        dialog_open(&sequencer_draw_render, &sequencer_render_hover, &sequencer_render_click);
        dialog_bindkeyboard(&sequencer_render_keyboard);
      }

      if (seq_ui[B_SEQPLAY]&1 && seq_render_start >= 0) {
        render_type=RENDER_LIVE;
        render_state=RENDER_START;
        sequencer_toggleplayback();
      }

      if (seq_ui[B_SAVE_SONG]) {
        songdir=dotfile_getvalue("songFileDir"); 
        filedialog_open(&songfd[FD_SAVE], "ksong", songdir);        
        songfd_active=FD_SAVE;
        dialog_open(&sequencer_draw_file, &sequencer_file_hover, &sequencer_file_click);
        dialog_bindkeyboard(&sequencer_file_keyboard);
        dialog_binddrag(&sequencer_file_drag);
        return;
      }
      if (seq_ui[B_LOAD_SONG]) {
        songdir=dotfile_getvalue("songFileDir"); 
        filedialog_open(&songfd[FD_LOAD], "ksong", songdir);        
        songfd_active=FD_LOAD;
        dialog_open(&sequencer_draw_file, &sequencer_file_hover, &sequencer_file_click);
        dialog_bindkeyboard(&sequencer_file_keyboard);        
        dialog_binddrag(&sequencer_file_drag);
        return;
      }
 
      if (seq_ui[B_LOOP]) render_live_loop^=1;

      // click on the slider?
      if (seqslide_hover) {
        seqslide_drag=1;
        seqslide_drag_xofs=x;
        seqslide_drag_start=seq_start;
      }

      // click on the timeline
      if (seq_timeline_hover>=0) {
        if (!seq_playing) {
          seq_render_start=seq_timeline_hover+seq_start; //(OUTPUTFREQ/(bpm*256/60))*((seq_timeline_hover+seq_start)*1024);
          seq_render_drag=1;
        }
        return;
      }

      // click on sequencer grid?
      if (seq_hover_ch>=0 && seq_hover_meas>=0 && seq_hover_ch<255) {
        if (sequencer_ispattern(seq_hover_ch, seq_hover_meas)) {
          // there's a pattern here, was shift down?
          m=glutGetModifiers();
          if (m==GLUT_ACTIVE_SHIFT) {
            // delete the pattern
           j=sequencer_patternstart(seq_hover_ch, seq_hover_meas);
           seq_pattern[seq_hover_ch][j]=-1;
           seq_repeat[seq_hover_ch][j]=1;
           seq_transpose[seq_hover_ch][j]=0;
           seq_patch[seq_hover_ch][j]=0;
          } else {
            //start dragging it
            seq_drag_active=1;
            seq_drag_grabpos=seq_hover_meas;
            seq_drag_droppos=seq_drag_grabpos;
            seq_drag_pattstart=sequencer_patternstart(seq_hover_ch, seq_hover_meas);
            seq_drag_pattch=seq_hover_ch;
          }
        } else {
          // left click on empty cell - do nothing?
        }
      }

      // click on modulator channel
      if (seq_hover_ch==255 && seq_hover_meas>=0) {
        console_post("Modulator commands are not yet implemented!");
      }
      
      // click on channel label
      if (seq_chlabel_hover>=0 && seq_chlabel_hover<255) {
        // pop up the channel settings dialog
          dialog_open(&sequencer_draw_channel, &sequencer_channel_hover, &sequencer_channel_click);
          dialog_bindkeyboard(&sequencer_channel_keyboard);
          sequencer_channel_hover(x, y); // call once to reset buttons
          return;
      }
      
      // click on modulator channel label
      if (seq_chlabel_hover==255) {
      }
    }
    if (state==GLUT_UP) {
      // if the slider is being dragged, release it
      if (seqslide_drag) { seqslide_drag=0; }

      if (seq_render_drag) {
        seq_render_end=sequencer_timelinepos(x, y)+seq_start;
        seq_timeline_hover=sequencer_timelinepos(x, y);
        seq_render_drag=0;
      }
    
      // check if there's a pattern being dragged. if yes, drop it here
      if (seq_drag_active) {
        seq_drag_active=0;
        dragdiff=seq_drag_droppos-seq_drag_grabpos;
        if (dragdiff!=0) {
          // move pattern data from drag_start to diff
          j=seq_drag_pattstart + (seq_drag_droppos-seq_drag_grabpos);
          pl=seq_repeat[seq_drag_pattch][seq_drag_pattstart]*pattlen[seq_pattern[seq_drag_pattch][seq_drag_pattstart]];
          if (j<0) j=0;
          if ( (j+pl) > seqsonglen) j=seqsonglen-pl;
          seq_pattern[seq_drag_pattch][j]=seq_pattern[seq_drag_pattch][seq_drag_pattstart];
          seq_repeat[seq_drag_pattch][j]=seq_repeat[seq_drag_pattch][seq_drag_pattstart];
          seq_transpose[seq_drag_pattch][j]=seq_transpose[seq_drag_pattch][seq_drag_pattstart];
          seq_patch[seq_drag_pattch][j]=seq_patch[seq_drag_pattch][seq_drag_pattstart];          
          seq_pattern[seq_drag_pattch][seq_drag_pattstart]=-1;
          for(i=1;i<pl;i++) seq_pattern[seq_drag_pattch][j+i]=-1;
          i=sequencer_cursorpos(x, y, &seq_hover_ch, &seq_hover_meas); // extra hovercheck to move the cursor as well
        } else {
          // pattern was clicked but not dragged anywhere - jump to pattern page and select the pattern
          i=sequencer_patternstart(seq_hover_ch, seq_hover_meas);
          
          cpatt=seq_pattern[seq_hover_ch][i];
          console_post("Patterns");
          cpage=3;
        }
      }
    } 
  }            

  if (button==GLUT_RIGHT_BUTTON) {
    if (state==GLUT_DOWN) {
    
      if (seq_timeline_hover>=0) {
        if (!seq_playing) {
          seq_render_end=seq_start+seq_timeline_hover; //(OUTPUTFREQ/(bpm*256/60))*((seq_timeline_hover+seq_start+1)*1024);
          if (seq_render_end < seq_render_start) {
            seq_render_start=-1; seq_render_end=-1;
          }  
        }
        return;
      }
      
      if (seq_hover_ch>=0 && seq_hover_meas>=0 && seq_hover_ch!=255) {
        if (sequencer_ispattern(seq_hover_ch, seq_hover_meas)) {
          // right-clicked a pattern so pop up the edit dialog
          i=sequencer_patternstart(seq_hover_ch, seq_hover_meas);
          seq_add_patt=seq_pattern[seq_hover_ch][i];
          seq_add_repeat=seq_repeat[seq_hover_ch][i];
          seq_add_transpose=seq_transpose[seq_hover_ch][i];
          seq_add_patch=seq_patch[seq_hover_ch][i];
          seq_add_editmode=1;
          seq_hover_meas=i;
          dialog_open(&sequencer_draw_pattern, &sequencer_pattern_hover, &sequencer_pattern_click);
          dialog_bindkeyboard(&sequencer_pattern_keyboard);          
        } else {
          // no pattern here yet, open edit dialog
          seq_add_patt=0;
          seq_add_repeat=1;
          seq_add_transpose=0;
          seq_add_editmode=0;
          seq_add_patch=0;
          dialog_open(&sequencer_draw_pattern, &sequencer_pattern_hover, &sequencer_pattern_click);
          dialog_bindkeyboard(&sequencer_pattern_keyboard);
        }
      }
      if (seq_hover_ch==255 && seq_hover_meas>=0) {
        // clicked on modulator channel - TODO
      }
    }
  }
}


void sequencer_keyboard(unsigned char key, int x, int y)
{
  int i,j,n,m;
  char *songdir;
  int seq_playing;
  
  seq_playing=((audiomode==AUDIOMODE_PLAY)&1);

  switch (key) {

    case ' ':
      if (seq_render_start >= 0) {
        render_type=RENDER_LIVE;
        render_state=RENDER_START;
        sequencer_toggleplayback();
        if (seq_playing) {
          // we stopped playback, so reset synths
          for(i=0;i<seqch;i++) audio_resetsynth(i);
        }
      }
      break;

    case 's':
    case 'S':
    songdir=dotfile_getvalue("songFileDir"); 
    filedialog_open(&songfd[FD_SAVE], "ksong", songdir);        
    songfd_active=FD_SAVE;
    dialog_open(&sequencer_draw_file, &sequencer_file_hover, &sequencer_file_click);
    dialog_bindkeyboard(&sequencer_file_keyboard);
    dialog_binddrag(&sequencer_file_drag);
    break;
    
    case 'l':
    case 'L':
    songdir=dotfile_getvalue("songFileDir"); 
    filedialog_open(&songfd[FD_LOAD], "ksong", songdir);        
    songfd_active=FD_LOAD;
    dialog_open(&sequencer_draw_file, &sequencer_file_hover, &sequencer_file_click);
    dialog_bindkeyboard(&sequencer_file_keyboard);        
    dialog_binddrag(&sequencer_file_drag);
    break;

    case 'c':
    case 'C':
    if (!seq_playing) {
      seq_render_start=-1; seq_render_end=-1;
    }
    break;
    
    case 'a':
    case 'A':
    if (!seq_playing) {
      for(i=0,m=0,seq_render_start=0;i<seqsonglen;i++) for(j=0;j<seqch;j++) {
        if (seq_pattern[j][i]>=0) {
          n=i+seq_repeat[j][i]*pattlen[seq_pattern[j][i]];
          if (n>m) m=n;
        }
      } 
      seq_render_end=m;  
    }
    break;
  }
}


void sequencer_draw(void)
{
  int i, j, k, sk, sl, osl, pl;
  float f, fs, fe, sx;
  char tmps[128];

 // divider lines
  for(j=0;j<=seqch;j++) {
    glColor4f(0.3, 0.3, 0.3, 0.8);
    glEnable(GL_LINE_STIPPLE);
    glLineStipple(1, 0xaaaa);
    glBegin(GL_LINES);
    glVertex2f(SEQUENCER_X,     SEQUENCER_Y+(j*SEQUENCER_CELLHEIGHT));
    glVertex2f(SEQUENCER_X+(seqsonglen-seq_start)*SEQUENCER_CELLWIDTH, SEQUENCER_Y+(j*SEQUENCER_CELLHEIGHT));
    glEnd();
    glDisable(GL_LINE_STIPPLE);
  }
  glColor4f(0.5, 0.5, 0.5, 0.8);
  glEnable(GL_LINE_STIPPLE);
  glLineStipple(1, 0xaaaa);
  glBegin(GL_LINES);
  glVertex2f(SEQUENCER_X,     SEQUENCER_Y+((seqch+2)*SEQUENCER_CELLHEIGHT));
  glVertex2f(SEQUENCER_X+(seqsonglen-seq_start)*SEQUENCER_CELLWIDTH, SEQUENCER_Y+((seqch+2)*SEQUENCER_CELLHEIGHT));
  glVertex2f(SEQUENCER_X,     SEQUENCER_Y+((seqch+3)*SEQUENCER_CELLHEIGHT));
  glVertex2f(SEQUENCER_X+(seqsonglen-seq_start)*SEQUENCER_CELLWIDTH, SEQUENCER_Y+((seqch+3)*SEQUENCER_CELLHEIGHT));
  glEnd();
  glDisable(GL_LINE_STIPPLE);
  for(i=0,j=seq_start;j<(seqsonglen+1);i++,j++) {
    glColor4f(0.2, 0.2, 0.2, 0.8);
    if ((j%4)==0) {
      glColor4f(0.5, 0.5, 0.5, 0.8);
    }
    if ((j%16)==0) {
      glColor4f(0.7, 0.7, 0.7, 0.8);
    }
    if ((j%4)==0) {
      sprintf(tmps, "%1d", j);
      render_text(tmps, 2.5+SEQUENCER_X+(i*SEQUENCER_CELLWIDTH), round(SEQUENCER_Y+((seqch+4)*SEQUENCER_CELLHEIGHT)), 2, 0xffa0a0a0, 0);    
    }
    glEnable(GL_LINE_STIPPLE);
    glLineStipple(1, 0xaaaa);
    glBegin(GL_LINES);
    glVertex2f(SEQUENCER_X+i*SEQUENCER_CELLWIDTH, round(SEQUENCER_Y));
    if ((j%4)==0) {
      glVertex2f(SEQUENCER_X+i*SEQUENCER_CELLWIDTH, round(SEQUENCER_Y+((seqch+4)*SEQUENCER_CELLHEIGHT)));
    } else {
      glVertex2f(SEQUENCER_X+i*SEQUENCER_CELLWIDTH, round(SEQUENCER_Y+((seqch+4)*SEQUENCER_CELLHEIGHT))); // was seqch+3
    }
    glEnd();
    glDisable(GL_LINE_STIPPLE);
  }

  // draw the patterns into the grid
  for(i=0;i<seqch;i++)
    for(j=0;j<seqsonglen;j++) {
      if (seq_pattern[i][j]>=0) {
        // pattern here, draw it
        sx=SEQUENCER_X+(j-seq_start)*SEQUENCER_CELLWIDTH;
        sl=seq_repeat[i][j]*pattlen[seq_pattern[i][j]]*SEQUENCER_CELLWIDTH;
        osl=sl;
        if ((sx+sl) > SEQUENCER_X) {
          sx++;
          if (sx<SEQUENCER_X) {
            sl-=(SEQUENCER_X-sx);
            sx=SEQUENCER_X;
          }
          if (sl > 0) {
            glColor4f(0.68, 0.33, 0.0, 0.5);
            glBegin(GL_QUADS);
            glVertex2f( sx+1,    SEQUENCER_Y+   i *SEQUENCER_CELLHEIGHT);
            glVertex2f( sx+sl, SEQUENCER_Y+   i *SEQUENCER_CELLHEIGHT);
            glVertex2f( sx+sl, SEQUENCER_Y+(1+i)*SEQUENCER_CELLHEIGHT-1 );
            glVertex2f( sx+1,    SEQUENCER_Y+(1+i)*SEQUENCER_CELLHEIGHT-1 );
            glEnd();        
            sprintf(tmps, "%02d", seq_pattern[i][j]);
            render_text(tmps, sx+1, SEQUENCER_Y+i*SEQUENCER_CELLHEIGHT+10, 2, 0x80c0c0c0, 0);
            glColor4f(0.68, 0.33, 0.0, 1.0);
            if (osl==sl) {
              glBegin(GL_LINE_LOOP);
            } else {
              glBegin(GL_LINE_STRIP);
            }
            glVertex2f( sx+1,    SEQUENCER_Y+   i *SEQUENCER_CELLHEIGHT);
            glVertex2f( sx+sl, SEQUENCER_Y+   i *SEQUENCER_CELLHEIGHT);
            glVertex2f( sx+sl, SEQUENCER_Y+(1+i)*SEQUENCER_CELLHEIGHT-1 );
            glVertex2f( sx+1,    SEQUENCER_Y+(1+i)*SEQUENCER_CELLHEIGHT-1 );
            glEnd();
            // splitter lines to mark repeats
            sl/=SEQUENCER_CELLWIDTH; osl/=SEQUENCER_CELLWIDTH;
            for(k=0;k<osl;k++) {
              if (k >= (osl-sl) && k>0 && k<osl) {
                if ( (k%pattlen[seq_pattern[i][j]])==0 ) {
                  // draw here
                  sk=k-(osl-sl);
                  glColor4f(0.68, 0.33, 0.0, 1.0);
                  glBegin(GL_LINES);
                  glVertex2f( sx+(sk*SEQUENCER_CELLWIDTH)+0.5, SEQUENCER_Y+ i*SEQUENCER_CELLHEIGHT);
                  glVertex2f( sx+(sk*SEQUENCER_CELLWIDTH)+0.5, SEQUENCER_Y+ i*SEQUENCER_CELLHEIGHT + (0.2*SEQUENCER_CELLHEIGHT));
                  glVertex2f( sx+(sk*SEQUENCER_CELLWIDTH)+0.5, SEQUENCER_Y+   i *SEQUENCER_CELLHEIGHT + (0.8*SEQUENCER_CELLHEIGHT));
                  glVertex2f( sx+(sk*SEQUENCER_CELLWIDTH)+0.5, SEQUENCER_Y+(1+i)*SEQUENCER_CELLHEIGHT-1 );
                  glEnd();
                }
              }
            }
          }
        }
      }
    }

  // if hovering on the grid, draw a cursor as guide
  if (seq_hover_ch>=0 && seq_hover_meas>=0 && seq_hover_ch<255) {
    // if a pattern is under the cursor, highlight the entire pattern. otherwise, highlight only the
    // single grid square involved
    if (sequencer_ispattern(seq_hover_ch, seq_hover_meas)) {
      i=seq_hover_ch;
      j=sequencer_patternstart(seq_hover_ch, seq_hover_meas);
      // pattern here, draw it
      sx=SEQUENCER_X+(j-seq_start)*SEQUENCER_CELLWIDTH;
      sl=seq_repeat[i][j]*pattlen[seq_pattern[i][j]]*SEQUENCER_CELLWIDTH;
      osl=sl;
      if ((sx+sl) > SEQUENCER_X) {
        sx++;
        if (sx<SEQUENCER_X) {
          sl-=(SEQUENCER_X-sx);
          sx=SEQUENCER_X;
        }
        if (sl > 0) {
          glColor4f(0.8, 0.8, 0.8, 0.8);
          glEnable(GL_LINE_STIPPLE);
          glLineStipple(1, 0x3333);
          if (osl==sl) {
            glBegin(GL_LINE_LOOP);
          } else {
            glBegin(GL_LINE_STRIP);
          }
          glVertex2f( sx+1,    SEQUENCER_Y+   i *SEQUENCER_CELLHEIGHT);
          glVertex2f( sx+sl, SEQUENCER_Y+   i *SEQUENCER_CELLHEIGHT);
          glVertex2f( sx+sl, SEQUENCER_Y+(1+i)*SEQUENCER_CELLHEIGHT-1 );
          glVertex2f( sx+1,    SEQUENCER_Y+(1+i)*SEQUENCER_CELLHEIGHT-1 );
          glEnd();                    
        glDisable(GL_LINE_STIPPLE);
        }
      }
    } else {
      // single square only
      glColor4f(0.8, 0.8, 0.8, 0.8);
      glEnable(GL_LINE_STIPPLE);
      glLineStipple(1, 0x3333);
      glBegin(GL_LINE_LOOP);
      glVertex2f( SEQUENCER_X+(seq_hover_meas-seq_start)  *SEQUENCER_CELLWIDTH, SEQUENCER_Y+   seq_hover_ch *SEQUENCER_CELLHEIGHT );
      glVertex2f( SEQUENCER_X+(seq_hover_meas-seq_start+1)*SEQUENCER_CELLWIDTH, SEQUENCER_Y+   seq_hover_ch *SEQUENCER_CELLHEIGHT );
      glVertex2f( SEQUENCER_X+(seq_hover_meas-seq_start+1)*SEQUENCER_CELLWIDTH, SEQUENCER_Y+(1+seq_hover_ch)*SEQUENCER_CELLHEIGHT );
      glVertex2f( SEQUENCER_X+(seq_hover_meas-seq_start)  *SEQUENCER_CELLWIDTH, SEQUENCER_Y+(1+seq_hover_ch)*SEQUENCER_CELLHEIGHT );
      glEnd();
      glDisable(GL_LINE_STIPPLE);
    }
  }
  if (seq_hover_ch==255 && seq_hover_meas>=0) {
    // hovering on modulator channel
    glColor4f(0.8, 0.8, 0.8, 0.8);
    glEnable(GL_LINE_STIPPLE);
    glLineStipple(1, 0x3333);
    glBegin(GL_LINE_LOOP);
    glVertex2f( SEQUENCER_X+(seq_hover_meas-seq_start)  *SEQUENCER_CELLWIDTH, SEQUENCER_Y+(2+seqch)*SEQUENCER_CELLHEIGHT );
    glVertex2f( SEQUENCER_X+(seq_hover_meas-seq_start+1)*SEQUENCER_CELLWIDTH, SEQUENCER_Y+(2+seqch)*SEQUENCER_CELLHEIGHT );
    glVertex2f( SEQUENCER_X+(seq_hover_meas-seq_start+1)*SEQUENCER_CELLWIDTH, SEQUENCER_Y+(3+seqch)*SEQUENCER_CELLHEIGHT );
    glVertex2f( SEQUENCER_X+(seq_hover_meas-seq_start)  *SEQUENCER_CELLWIDTH, SEQUENCER_Y+(3+seqch)*SEQUENCER_CELLHEIGHT );
    glEnd();
    glDisable(GL_LINE_STIPPLE);
  }

  // "ghost" if dragging a pattern on grid
  if (seq_drag_active) {
    i=seq_drag_pattch;
    j=seq_drag_pattstart + (seq_drag_droppos-seq_drag_grabpos);
    pl=seq_repeat[i][seq_drag_pattstart]*pattlen[seq_pattern[i][seq_drag_pattstart]];
    if (j<0) j=0;
    if ( (j+pl) > seqsonglen) j=seqsonglen-pl;
    sx=SEQUENCER_X+(j-seq_start)*SEQUENCER_CELLWIDTH;
    sl=pl*SEQUENCER_CELLWIDTH;
    osl=sl;
    if ((sx+sl) > SEQUENCER_X) {
      if (sx<SEQUENCER_X) {
        sl-=(SEQUENCER_X-sx);
        sx=SEQUENCER_X;
      }
      if (sl > 0) {
        glColor4f(0.8, 0.8, 0.8, 0.4);
        glBegin(GL_QUADS);
        glVertex2f( sx+1,    SEQUENCER_Y+   i *SEQUENCER_CELLHEIGHT);
        glVertex2f( sx+sl, SEQUENCER_Y+   i *SEQUENCER_CELLHEIGHT);
        glVertex2f( sx+sl, SEQUENCER_Y+(1+i)*SEQUENCER_CELLHEIGHT-1 );
        glVertex2f( sx+1,    SEQUENCER_Y+(1+i)*SEQUENCER_CELLHEIGHT-1 );
        glEnd();
      }
    }
  }

  // draw the render start and end positions
  if (seq_render_start >= 0 && seq_render_start >= seq_start) {
    f=seq_render_start - seq_start;
    glColor4f(0.0, 1.0, 0.0, 1.0);
    glBegin(GL_LINES);
    glVertex2f(SEQUENCER_X+f*SEQUENCER_CELLWIDTH, SEQUENCER_Y);
    glVertex2f(SEQUENCER_X+f*SEQUENCER_CELLWIDTH, SEQUENCER_Y+(seqch+3)*SEQUENCER_CELLHEIGHT);
    glEnd();
  }
  if (seq_render_end >= 0 && seq_render_end >= seq_start) {
    f=seq_render_end - seq_start;
    glColor4f(1.0, 0.0, 0.0, 1.0);
    glBegin(GL_LINES);
    glVertex2f(SEQUENCER_X+f*SEQUENCER_CELLWIDTH, SEQUENCER_Y);
    glVertex2f(SEQUENCER_X+f*SEQUENCER_CELLWIDTH, SEQUENCER_Y+(seqch+3)*SEQUENCER_CELLHEIGHT);
    glEnd();
  }
  if (seq_render_start >= 0 && 
      seq_render_end >= 0 && 
      seq_render_start < seq_render_end &&
      seq_render_end > seq_start)
  {
    fs=seq_render_start-seq_start; 
    if (fs<0) fs=0;
    fe=seq_render_end-seq_start;
    if ((fe-fs)>0) {
      glColor4f(0.5, 0.5, 0.6, 0.3);
      glBegin(GL_QUADS);
        glVertex2f(SEQUENCER_X+fs*SEQUENCER_CELLWIDTH, SEQUENCER_Y);
        glVertex2f(SEQUENCER_X+fs*SEQUENCER_CELLWIDTH, SEQUENCER_Y+(seqch+3)*SEQUENCER_CELLHEIGHT);
        glVertex2f(SEQUENCER_X+fe*SEQUENCER_CELLWIDTH, SEQUENCER_Y+(seqch+3)*SEQUENCER_CELLHEIGHT);
        glVertex2f(SEQUENCER_X+fe*SEQUENCER_CELLWIDTH, SEQUENCER_Y);
      glEnd();
    }
  }

  // timeline cursor
  if (seq_timeline_hover>=0) {
      i=seq_timeline_hover;
      glColor4f(1.0, 1.0, 1.0, 1.0);
      glEnable(GL_LINE_STIPPLE);
      glLineStipple(1, 0xf0f0);
      glBegin(GL_LINES);
      glVertex2f(SEQUENCER_X+i*SEQUENCER_CELLWIDTH, SEQUENCER_Y);
      glVertex2f(SEQUENCER_X+i*SEQUENCER_CELLWIDTH, SEQUENCER_Y+(seqch+3)*SEQUENCER_CELLHEIGHT);
      glEnd();
      glDisable(GL_LINE_STIPPLE);
  }

  // live playback position
  if (audiomode==AUDIOMODE_PLAY && (render_state == RENDER_LIVE || render_state == RENDER_LIVE_COMPLETE)) {
    if (render_state==RENDER_LIVE) {
      i=(render_pos / (OUTPUTFREQ/(bpm*256/60))) >> 6; // calc patternpos from sample index
      i+=seq_render_start*16;
      if (i >= 0 && (i/16.0f) >= seq_start) {
        f=(float)(i)/16.0f - seq_start;
        glColor4f(1.0, 1.0, 1.0, 0.5);
        glBegin(GL_LINES);
        glVertex2f(SEQUENCER_X+f*SEQUENCER_CELLWIDTH, SEQUENCER_Y);
        glVertex2f(SEQUENCER_X+f*SEQUENCER_CELLWIDTH, SEQUENCER_Y+(seqch+3)*SEQUENCER_CELLHEIGHT);
        glEnd();
      }
    }

    i=(render_playpos / (OUTPUTFREQ/(bpm*256/60))) >> 6; // calc patternpos from sample index
    i+=seq_render_start*16;
    if (i >= 0 && (i/16.0f) >= seq_start) {
        f=(float)(i)/16.0f - seq_start;
      glColor4f(1.0, 1.0, 1.0, 1.0);
      glBegin(GL_LINES);
      glVertex2f(SEQUENCER_X+f*SEQUENCER_CELLWIDTH, SEQUENCER_Y);
      glVertex2f(SEQUENCER_X+f*SEQUENCER_CELLWIDTH, SEQUENCER_Y+(seqch+3)*SEQUENCER_CELLHEIGHT);
      glEnd();
    }
  }  
  
  // channel numbers
  for(j=0;j<seqch;j++) {
    sprintf(tmps, "ch %02d", j+1);
    if (j==seq_chlabel_hover) {
      glColor4f(0.8, 0.8, 0.8, 0.4);
      glBegin(GL_QUADS);
      glVertex2f(1, SEQUENCER_Y+j*SEQUENCER_CELLHEIGHT);
      glVertex2f(SEQUENCER_X-1, SEQUENCER_Y+j*SEQUENCER_CELLHEIGHT);            
      glVertex2f(SEQUENCER_X-1, SEQUENCER_Y+(j+1)*SEQUENCER_CELLHEIGHT);
      glVertex2f(1, SEQUENCER_Y+(j+1)*SEQUENCER_CELLHEIGHT);
      glEnd();
    }
    if (j==seq_hover_ch) {
      render_text(tmps, 5, SEQUENCER_Y+10.5+SEQUENCER_CELLHEIGHT*j, 2, 0xffad5400, 0);  
    } else {
      render_text(tmps, 5, SEQUENCER_Y+10.5+SEQUENCER_CELLHEIGHT*j, 2, 0xffc0c0c0, 0);  
    }
  }
  if (seq_chlabel_hover==255) {
      j=seqch+2;
      glColor4f(0.8, 0.8, 0.8, 0.4);
      glBegin(GL_QUADS);
      glVertex2f(1, SEQUENCER_Y+j*SEQUENCER_CELLHEIGHT);
      glVertex2f(SEQUENCER_X-1, SEQUENCER_Y+j*SEQUENCER_CELLHEIGHT);            
      glVertex2f(SEQUENCER_X-1, SEQUENCER_Y+(j+1)*SEQUENCER_CELLHEIGHT);
      glVertex2f(1, SEQUENCER_Y+(j+1)*SEQUENCER_CELLHEIGHT);
      glEnd();
  }
  render_text("mod", 5, SEQUENCER_Y+10.5+SEQUENCER_CELLHEIGHT*(seqch+2), 2, 0xffc0c0c0, 0);

  // draw the horizontal slider
  draw_hslider(SEQUENCER_X, SEQUENCER_Y+(seqch+5)*SEQUENCER_CELLHEIGHT, (DS_WIDTH-(SEQUENCER_X+6)), 12,
   seq_start, (DS_WIDTH-(SEQUENCER_X+4))/SEQUENCER_CELLWIDTH, seqsonglen, seqslide_hover);

  // render area selection and playback
  if (audiomode==AUDIOMODE_COMPOSING) seq_ui[B_SEQPLAY]&=1;
  draw_button(440, DS_HEIGHT-14, 16, "A", seq_ui[B_REWIND]);
  draw_button(462, DS_HEIGHT-14, 16, "C", seq_ui[B_CLEAR]);
  draw_textbox(526, DS_HEIGHT-14, 16, 48, "play", seq_ui[B_SEQPLAY]);
  draw_textbox(578, DS_HEIGHT-14, 16, 48, "render", seq_ui[B_RENDER]);
  glColor4f(0, 0, 0, 0.4);
  glBegin(GL_QUADS);
  if (seq_render_start < 0) {
    glVertex2f(526-24, (DS_HEIGHT-14)-8);  glVertex2f(526+24, (DS_HEIGHT-14)-8);
    glVertex2f(526+24, (DS_HEIGHT-14)+8);  glVertex2f(526-24, (DS_HEIGHT-14)+8);
  }
  if (seq_render_start < 0 || seq_render_end < 0 || seq_render_start >= seq_render_end) {
    glVertex2f(578-24, (DS_HEIGHT-14)-8);  glVertex2f(578+24, (DS_HEIGHT-14)-8);
    glVertex2f(578+24, (DS_HEIGHT-14)+8);  glVertex2f(578-24, (DS_HEIGHT-14)+8);
    glEnd();
  }

  // draw the ui elements on the seq page
  draw_button(14, DS_HEIGHT-14, 16, "<<", seq_ui[B_DECCH]);
  sprintf(tmps, "%02d ch", seqch);
  draw_textbox(47, DS_HEIGHT-14, 16, 36, tmps, 0);
  draw_button(80, DS_HEIGHT-14, 16, ">>", seq_ui[B_ADDCH]);
  draw_button(114,  DS_HEIGHT-14, 16, "<<", seq_ui[B_BPMDN]);
  sprintf(tmps, "%d bpm", bpm);
  draw_textbox(151, DS_HEIGHT-14, 16, 46, tmps, 0);
  draw_button(188,  DS_HEIGHT-14, 16, ">>", seq_ui[B_BPMUP]);
  draw_button(222,  DS_HEIGHT-14, 16, "<<", seq_ui[B_SSHORTER]);
  sprintf(tmps, "%d msr", seqsonglen);
  draw_textbox(263, DS_HEIGHT-14, 16, 54, tmps, 0);
  draw_button(304,  DS_HEIGHT-14, 16, ">>", seq_ui[B_SLONGER]);
  
  draw_button(350, DS_HEIGHT-14, 16, "S", seq_ui[B_SAVE_SONG]);
  draw_button(372, DS_HEIGHT-14, 16, "L", seq_ui[B_LOAD_SONG]);

  draw_textbox(630, DS_HEIGHT-14, 16, 48, "loop", seq_ui[B_LOOP] | (render_live_loop ? 2 : 0));

  draw_button(394, DS_HEIGHT-14, 16, "N", seq_ui[B_NEWSONG]);
  
}







//
// pattern edit dialog
//
void sequencer_draw_pattern(void)
{
  char tmps[128];

  draw_textbox((DS_WIDTH/2), (DS_HEIGHT/2), 150, 240, "", 0);
  if (seq_add_editmode) { sprintf(tmps, "Edit pattern (ch%02d)",seq_hover_ch+1); } else { sprintf(tmps, "Add pattern (ch%02d)", seq_hover_ch+1); }
  render_text(tmps, (DS_WIDTH/2)-116, (DS_HEIGHT/2)-62, 0, 0xffb05500, 0);
  render_text("right click to close", (DS_WIDTH/2)+6, (DS_HEIGHT/2)+85, 2, 0xffc0c0c0, 0);

  render_text("pattern", (DS_WIDTH/2)-59, (DS_HEIGHT/2)-42, 2, 0xffc0c0c0, 1);
  sprintf(tmps, "%02d", seq_add_patt);
  draw_textbox((DS_WIDTH/2)-59, (DS_HEIGHT/2)-30, 16, 62, tmps, 0);
  draw_button((DS_WIDTH/2)-16, (DS_HEIGHT/2)-30, 16, ">>", seq_ui[B_PATT_ADDNUM]);
  draw_button((DS_WIDTH/2)-102, (DS_HEIGHT/2)-30, 16, "<<", seq_ui[B_PATT_DECNUM]);

  render_text("repeat", (DS_WIDTH/2)+59, (DS_HEIGHT/2)-42, 2, 0xffc0c0c0, 1);
  sprintf(tmps, "%d", seq_add_repeat);
  draw_textbox((DS_WIDTH/2)+59, (DS_HEIGHT/2)-30, 16, 62, tmps, 0);
  draw_button((DS_WIDTH/2)+102, (DS_HEIGHT/2)-30, 16, ">>", seq_ui[B_PATT_ADDREP]);
  draw_button((DS_WIDTH/2)+16, (DS_HEIGHT/2)-30, 16, "<<", seq_ui[B_PATT_DECREP]);

  render_text("transpose:", (DS_WIDTH/2)-104, (DS_HEIGHT/2)-1, 2, 0xffc0c0c0, 0);
  sprintf(tmps, "%d semitones", seq_add_transpose);
  draw_textbox((DS_WIDTH/2)+43, (DS_HEIGHT/2)-4, 16, 94, tmps, 0);
  draw_button((DS_WIDTH/2)+102, (DS_HEIGHT/2)-4, 16, ">>", seq_ui[B_PATT_ADDTRANS]);
  draw_button((DS_WIDTH/2)-16, (DS_HEIGHT/2)-4, 16, "<<", seq_ui[B_PATT_DECTRANS]);

  render_text("patch", (DS_WIDTH/2), (DS_HEIGHT/2)+20, 2, 0xffc0c0c0, 1);
  sprintf(tmps, "%02d: %s", seq_add_patch, patchname[seq_synth[seq_hover_ch]][seq_add_patch]);
  draw_textbox((DS_WIDTH/2), (DS_HEIGHT/2)+32, 16, 180, tmps, 0);
  draw_button((DS_WIDTH/2)+102, (DS_HEIGHT/2)+32, 16, ">>", seq_ui[B_PATT_NEXTPATCH]);
  draw_button((DS_WIDTH/2)-102, (DS_HEIGHT/2)+32, 16, "<<", seq_ui[B_PATT_PREVPATCH]);

  if (seq_add_editmode) { sprintf(tmps, "save changes"); } else { sprintf(tmps, "add pattern"); }  
  draw_textbox((DS_WIDTH/2)+40, (DS_HEIGHT/2)+60, 16, 100, tmps, seq_ui[B_PATT_ADD]);
}


void sequencer_pattern_hover(int x, int y)
{
  seq_ui[B_PATT_ADDNUM]=hovertest_box(x, y, (DS_WIDTH/2)-16, (DS_HEIGHT/2)-30, 16, 16);
  seq_ui[B_PATT_DECNUM]=hovertest_box(x, y, (DS_WIDTH/2)-102, (DS_HEIGHT/2)-30, 16, 16);
  seq_ui[B_PATT_ADDREP]=hovertest_box(x, y,(DS_WIDTH/2)+102, (DS_HEIGHT/2)-30, 16, 16);
  seq_ui[B_PATT_DECREP]=hovertest_box(x, y,(DS_WIDTH/2)+16, (DS_HEIGHT/2)-30, 16, 16);
  seq_ui[B_PATT_ADDTRANS]=hovertest_box(x, y,(DS_WIDTH/2)+102, (DS_HEIGHT/2)-4, 16, 16);
  seq_ui[B_PATT_DECTRANS]=hovertest_box(x, y,(DS_WIDTH/2)-16, (DS_HEIGHT/2)-4, 16, 16);
  seq_ui[B_PATT_ADD]=hovertest_box(x, y,(DS_WIDTH/2)+40, (DS_HEIGHT/2)+60, 16, 100);
  seq_ui[B_PATT_NEXTPATCH]=hovertest_box(x, y, (DS_WIDTH/2)+102, (DS_HEIGHT/2)+32, 16, 16);
  seq_ui[B_PATT_PREVPATCH]=hovertest_box(x, y, (DS_WIDTH/2)-102, (DS_HEIGHT/2)+32, 16, 16);  
}


void sequencer_pattern_click(int button, int state, int x, int y)
{
  int i;

  if (state==GLUT_DOWN && !hovertest_box(x,y,(DS_WIDTH/2),(DS_HEIGHT/2),150,240 )) {
    dialog_close(); return;
  }

  if (button==GLUT_RIGHT_BUTTON && state==GLUT_DOWN && hovertest_box(x,y,(DS_WIDTH/2),(DS_HEIGHT/2),150,240 )) {
    dialog_close(); return; 
  }
  
  if (button==GLUT_LEFT_BUTTON) {
    if (state==GLUT_DOWN) {
      if (seq_ui[B_PATT_ADDNUM])   { if (seq_add_patt<MAX_PATTERN) seq_add_patt++; }
      if (seq_ui[B_PATT_DECNUM])   { if (seq_add_patt>0) seq_add_patt--; }
      if (seq_ui[B_PATT_ADDREP])   { if (seq_add_repeat<64) seq_add_repeat++; }
      if (seq_ui[B_PATT_DECREP])   { if (seq_add_repeat>1) seq_add_repeat--; }
      if (seq_ui[B_PATT_ADDTRANS]) { if (seq_add_transpose<48) seq_add_transpose++; }
      if (seq_ui[B_PATT_DECTRANS]) { if (seq_add_transpose>-48) seq_add_transpose--; }
      if (seq_ui[B_PATT_NEXTPATCH]) { if (seq_add_patch<MAX_PATCHES) seq_add_patch++; }
      if (seq_ui[B_PATT_PREVPATCH]) { if (seq_add_patch>0) seq_add_patch--; }
    
      if (seq_ui[B_PATT_ADD]) {
        // add the pattern and close the dialog
        for(i=0;i<seq_add_repeat;i++) seq_pattern[seq_hover_ch][seq_hover_meas+i]=-1; // reset the underlying patterns
        seq_pattern[seq_hover_ch][seq_hover_meas]=seq_add_patt;
        seq_repeat[seq_hover_ch][seq_hover_meas]=seq_add_repeat;
        seq_transpose[seq_hover_ch][seq_hover_meas]=seq_add_transpose;
        seq_patch[seq_hover_ch][seq_hover_meas]=seq_add_patch;
        
        dialog_close();
        return;
      }
    }
  }
}


void sequencer_pattern_keyboard(unsigned char key, int x, int y)
{
}    









//
// channel settings dialog
//
void sequencer_draw_channel(void)
{
  char tmps[128];

  draw_textbox((DS_WIDTH/2), (DS_HEIGHT/2), 160, 240, "", 0);
  
  sprintf(tmps, "Channel %02d", seq_chlabel_hover+1);
  render_text(tmps, (DS_WIDTH/2)-108, (DS_HEIGHT/2)-60, 0, 0xffb05500, 0);
  render_text("right click to close", (DS_WIDTH/2)-2, (DS_HEIGHT/2)+78, 2, 0xffc0c0c0, 0);

  sprintf(tmps, "%02d: %s", seq_synth[seq_chlabel_hover], synthname[seq_synth[seq_chlabel_hover]]);
  draw_textbox((DS_WIDTH/2), (DS_HEIGHT/2)-36, 16, 180, tmps, 0);
  draw_button((DS_WIDTH/2)+102, (DS_HEIGHT/2)-36, 16, ">>", seq_ui[B_CHAN_NEXTSYNTH]);
  draw_button((DS_WIDTH/2)-102, (DS_HEIGHT/2)-36, 16, "<<", seq_ui[B_CHAN_PREVSYNTH]);

  draw_textbox((DS_WIDTH/2),    (DS_HEIGHT/2)-12, 8, 180, "Envelope hard restart", seq_ui[B_ENVRESTART]);
  draw_textbox((DS_WIDTH/2),    (DS_HEIGHT/2)+4,  8, 180, "Oscillator hard restart", seq_ui[B_VCORESTART]);
  draw_textbox((DS_WIDTH/2),    (DS_HEIGHT/2)+20, 8, 180, "LFO hard restart", seq_ui[B_LFORESTART]);
}


void sequencer_channel_hover(int x, int y)
{
  seq_ui[B_CHAN_NEXTSYNTH]=hovertest_box(x, y, (DS_WIDTH/2)+102, (DS_HEIGHT/2)-36, 16, 16);
  seq_ui[B_CHAN_PREVSYNTH]=hovertest_box(x, y, (DS_WIDTH/2)-102, (DS_HEIGHT/2)-36, 16, 16);

  seq_ui[B_ENVRESTART]=hovertest_box(x, y, (DS_WIDTH/2), (DS_HEIGHT/2)-8, 8, 180);
  seq_ui[B_ENVRESTART]|=seq_restart[seq_chlabel_hover]&SEQ_RESTART_ENV ? 2 : 0 ;
  seq_ui[B_VCORESTART]=hovertest_box(x, y, (DS_WIDTH/2), (DS_HEIGHT/2)+4, 8, 180);
  seq_ui[B_VCORESTART]|=seq_restart[seq_chlabel_hover]&SEQ_RESTART_VCO ? 2 : 0 ;  
  seq_ui[B_LFORESTART]=hovertest_box(x, y, (DS_WIDTH/2), (DS_HEIGHT/2)+20, 8, 180);
  seq_ui[B_LFORESTART]|=seq_restart[seq_chlabel_hover]&SEQ_RESTART_LFO ? 2 : 0 ;
}


void sequencer_channel_click(int button, int state, int x, int y)
{
  if (state==GLUT_DOWN && !hovertest_box(x,y,(DS_WIDTH/2),(DS_HEIGHT/2),150,240 )) {
    kmm_gcollect();
    dialog_close();
    return;
  }

  if (button==GLUT_LEFT_BUTTON) {
    if (state==GLUT_DOWN) {
      if (seq_ui[B_CHAN_NEXTSYNTH]) { 
        if (seq_synth[seq_chlabel_hover]<MAX_SYNTH) {
          synth_lockaudio();
          seq_synth[seq_chlabel_hover]++;
          kmm_gcollect();
          synth_releaseaudio();
        }
      }
      if (seq_ui[B_CHAN_PREVSYNTH]) {
        if (seq_synth[seq_chlabel_hover]>0) {
          synth_lockaudio();
          seq_synth[seq_chlabel_hover]--;
          kmm_gcollect();
          synth_releaseaudio();
        }
      }
      
      if (seq_ui[B_ENVRESTART]&1) seq_restart[seq_chlabel_hover]^=SEQ_RESTART_ENV;
      if (seq_ui[B_VCORESTART]&1) seq_restart[seq_chlabel_hover]^=SEQ_RESTART_VCO;
      if (seq_ui[B_LFORESTART]&1) seq_restart[seq_chlabel_hover]^=SEQ_RESTART_LFO;

      sequencer_channel_hover(x,y);      
      return;
    }
  }

  if (button==GLUT_RIGHT_BUTTON && hovertest_box(x,y,(DS_WIDTH/2),(DS_HEIGHT/2),150,240 )) {
    kmm_gcollect();
    dialog_close(); return; 
  }
}


void sequencer_channel_keyboard(unsigned char key, int x, int y)
{
  if (key==27) {
    kmm_gcollect();
    dialog_close(); return; 
  }  
}    









//
// song render dialog
//
void sequencer_draw_render(void)
{
  char tmps[128];
  float rf;

  if (render_state==RENDER_COMPLETE) {
//if (render_pos > (0.3*render_bufferlen)) {
    dialog_close();
    dialog_open(&sequencer_draw_preview, &sequencer_preview_hover, &sequencer_preview_click);
    dialog_bindkeyboard(&sequencer_preview_keyboard);            
    return;
  }

  draw_textbox((DS_WIDTH/2), (DS_HEIGHT/2), 80, 240, "", 0);
  
  render_text("Rendering...", (DS_WIDTH/2)-108, (DS_HEIGHT/2)-20, 0, 0xffb05500, 0);
  render_text("esc/right click to abort", (DS_WIDTH/2)+118, (DS_HEIGHT/2)+38, 2, 0xffc0c0c0, 2);

  rf=(float)(render_pos) / (float)(render_bufferlen);
  draw_textbox((DS_WIDTH/2), (DS_HEIGHT/2)+4, 16, 180, "", 0); 
  glColor4f(0.68f, 0.33f, 0.0f, 0.94f);
  glBegin(GL_QUADS);
  glVertex2f((DS_WIDTH/2)-90, (DS_HEIGHT/2)-4);
  glVertex2f((DS_WIDTH/2)-90, (DS_HEIGHT/2)+12);
  glVertex2f((DS_WIDTH/2)-90 + rf*180, (DS_HEIGHT/2)+12);
  glVertex2f((DS_WIDTH/2)-90 + rf*180, (DS_HEIGHT/2)-4);
  glEnd();
  sprintf(tmps, "%6.2f%%", rf*100);
  render_text(tmps, (DS_WIDTH/2), (DS_HEIGHT/2)+7, 2, 0xffffffff, 1);
  
}


void sequencer_render_hover(int x, int y)
{
}


void sequencer_render_click(int button, int state, int x, int y)
{
  if (button==GLUT_LEFT_BUTTON) {
    if (state==GLUT_DOWN) {
    }
  }

  if (button==GLUT_RIGHT_BUTTON && hovertest_box(x,y,(DS_WIDTH/2),(DS_HEIGHT/2),150,240 )) {
    audiomode=AUDIOMODE_COMPOSING;
    render_state=RENDER_STOPPED;
    dialog_close(); return; 
  }
}


void sequencer_render_keyboard(unsigned char key, int x, int y)
{
  if (key==27) {
    audiomode=AUDIOMODE_COMPOSING;
    render_state=RENDER_STOPPED;
    dialog_close(); return; 
  }
}    










//
// song render dialog
//
void sequencer_draw_preview(void)
{
  int i;
  long spos;
  float s;
  char tmps[128];

  seq_ui[B_PREVIEW_PLAY]&=1;
  if (render_state==RENDER_PLAYBACK) seq_ui[B_PREVIEW_PLAY]|=2;
  
  draw_textbox((DS_WIDTH/2), (DS_HEIGHT/2), 210, (DS_WIDTH*0.8), "", 0);
  render_text("esc/right click to close", (DS_WIDTH/2)+(DS_WIDTH*0.4)-2, (DS_HEIGHT/2)+101, 2, 0xffc0c0c0, 2);

  // sample preview
  draw_textbox((DS_WIDTH/2), (DS_HEIGHT/2)-18, 160, (DS_WIDTH*0.8)-20, "", 0);
  glColor4f(0, 0, 0, 1.0);
  glBegin(GL_QUADS);
  glVertex2f((DS_WIDTH*0.1)+10, (DS_HEIGHT/2)-98);
  glVertex2f((DS_WIDTH*0.1)+10, (DS_HEIGHT/2)+62);
  glVertex2f((DS_WIDTH*0.9)-10, (DS_HEIGHT/2)+62);
  glVertex2f((DS_WIDTH*0.9)-10, (DS_HEIGHT/2)-98);
  glEnd();

  glBegin(GL_LINE_STRIP);
  for(i=0;i<((DS_WIDTH*0.8)-20);i++) {
    glColor4f(0.68f, 0.33f, 0.0f, 0.94f);
    spos=(long)(((float)(i) / ((DS_WIDTH*0.8f)-20.0f)) * render_bufferlen);
    s=(float)(render_buffer[spos*2])/32766.0f;
    s*=80.0f;
    if (s>80.0) s=80.0;
    if (s<-80.0) s=-80.0;
    glVertex2f((DS_WIDTH*0.1)+10+i, (DS_HEIGHT/2)-18+s);
    glVertex2f((DS_WIDTH*0.1)+10+i, (DS_HEIGHT/2)-18-s);
  }
  glEnd();
  
  // mark playpos
  s=((float)(render_playpos)/(float)(render_bufferlen))*((DS_WIDTH*0.8)-20);
  glColor4f(0.8f, 0.8f, 0.8f, 0.8f);
  glBegin(GL_LINE_STRIP);  
    glVertex2f((DS_WIDTH*0.1)+10+s, (DS_HEIGHT/2)-97);
    glVertex2f((DS_WIDTH*0.1)+10+s, (DS_HEIGHT/2)+61);
  glEnd();
  
  // scale
  glColor4f(0.8f, 0.8f, 0.8f, 0.8f);
  glBegin(GL_LINE_STRIP);
  glVertex2f((DS_WIDTH*0.1)+10.5, (DS_HEIGHT/2)+70);
  glVertex2f((DS_WIDTH*0.1)+10.5, (DS_HEIGHT/2)+67.5);
  glVertex2f((DS_WIDTH*0.9)-10.5, (DS_HEIGHT/2)+67.5);
  glVertex2f((DS_WIDTH*0.9)-10.5, (DS_HEIGHT/2)+70);
  glEnd();
  render_text("0 s", (DS_WIDTH*0.1)+10, (DS_HEIGHT/2)+76, 2, 0xffffffff, 0);
  s=(float)(render_bufferlen)/(float)(OUTPUTFREQ);
  sprintf(tmps, "%6.2f s", s);
  render_text(tmps, (DS_WIDTH*0.9)-10, (DS_HEIGHT/2)+76, 2, 0xffffffff, 2);

  // ui buttons for the preview playback window
  s=(float)(render_playpos)/(float)(OUTPUTFREQ);
  sprintf(tmps, "%6.2f s", s);  
  draw_button((DS_WIDTH/2)-40, (DS_HEIGHT/2)+92, 16, "i<", seq_ui[B_PREVIEW_REWIND]);
  draw_textbox((DS_WIDTH/2), (DS_HEIGHT/2)+92, 16, 52, tmps, seq_ui[B_PREVIEW_PLAY]);  
  draw_button((DS_WIDTH/2)+100, (DS_HEIGHT/2)+92, 16, "w", seq_ui[B_PREVIEW_EXPORT]);
}


void sequencer_preview_hover(int x, int y)
{
  seq_ui[B_PREVIEW_REWIND]=hovertest_box(x, y, (DS_WIDTH/2)-40, (DS_HEIGHT/2)+92, 16, 16);
  seq_ui[B_PREVIEW_PLAY]=hovertest_box(x, y, (DS_WIDTH/2), (DS_HEIGHT/2)+92, 16, 52);
  if (render_state==RENDER_PLAYBACK) seq_ui[B_PREVIEW_PLAY]|=2;

  seq_ui[B_PREVIEW_EXPORT]=hovertest_box(x, y, (DS_WIDTH/2)+100, (DS_HEIGHT/2)+92, 16, 16);
  
  // hovering on render preview
  seq_render_hover=-1;
  if( x >= ((DS_WIDTH*0.1)+10) && x < ((DS_WIDTH*0.9)-10) &&
      y >= ((DS_HEIGHT/2)-98)  && y < ((DS_HEIGHT/2)+62)) {
    seq_render_hover= ((float)(x-((DS_WIDTH*0.1)+10)) / (float)((DS_WIDTH*0.8)-20)) * render_bufferlen;
  }
  
}


void sequencer_preview_click(int button, int state, int x, int y)
{
  if (button==GLUT_LEFT_BUTTON) {
    if (state==GLUT_DOWN) {
      if (seq_ui[B_PREVIEW_PLAY]&1) {
        // toggle playback
        if (render_state==RENDER_PLAYBACK) {
          render_state=RENDER_COMPLETE;
        } else {
          render_state=RENDER_PLAYBACK;
        }
        return;
      }
      if (seq_ui[B_PREVIEW_REWIND]) { render_playpos=0; return; }

      // wav dump button
      if (seq_ui[B_PREVIEW_EXPORT]) { 
        audio_exportwav();
        return;
      }
      
      if (seq_render_hover>=0) { render_playpos=seq_render_hover; return; }
    }
  }

  if (button==GLUT_RIGHT_BUTTON && hovertest_box(x,y,(DS_WIDTH/2),(DS_HEIGHT/2),210,(DS_WIDTH*0.8) )) {
    audiomode=AUDIOMODE_COMPOSING;
    render_state=RENDER_STOPPED;
    dialog_close(); return; 
  }
}


void sequencer_preview_keyboard(unsigned char key, int x, int y)
{
  if (key==' ') { // spacebar toggles playback
    if (render_state==RENDER_PLAYBACK) {render_state=RENDER_COMPLETE;} else { render_state=RENDER_PLAYBACK; }
    return;  
  }
  if (key==27) {
    audiomode=AUDIOMODE_COMPOSING;
    render_state=RENDER_STOPPED;
    dialog_close(); return; 
  }
}    





//
// file dialog functions
//
void sequencer_draw_file(void)
{
  filedialog_draw(&songfd[songfd_active]);
}
void sequencer_file_hover(int x, int y)
{
  filedialog_hover(&songfd[songfd_active], x, y);
}
void sequencer_file_click(int button, int state, int x, int y)
{
  filedialog_click(&songfd[songfd_active], button, state, x, y);
  sequencer_file_checkstate();
}
void sequencer_file_keyboard(unsigned char key, int x, int y)
{
  filedialog_keyboard(&songfd[songfd_active], key);
  sequencer_file_checkstate();
}
void sequencer_file_drag(int x, int y)
{
  filedialog_drag(&songfd[songfd_active], x, y);
}
void sequencer_file_checkstate(void)
{
  int r, i;
  char fn[255], tmps[255];;

  if (songfd[songfd_active].exitstate) {
    dialog_close();
    audiomode=AUDIOMODE_MUTE;
    if (songfd[songfd_active].exitstate==FDEXIT_OK) {
      // clicked ok, do stuff with the filename
      sprintf(fn, "%s", songfd[songfd_active].fullpath);
      if (songfd_active==FD_SAVE) {
        r=save_ksong(fn);
        if (r) {
          console_post("Error while saving song!");
        } else {
          sprintf(tmps, "Song saved as %s", fn);
          console_post(tmps);
        }
      }
      if (songfd_active==FD_LOAD) {
        synth_lockaudio();
        r=load_ksong(fn);
        if (r) {
          console_post("Error while loading song!");
        } else {
          // song loaded ok, clean up and reload
          for(i=0;i<MAX_SYNTH;i++) synth_clear(i);
          patch_init();
          pattern_init();
          sequencer_clearsong();
          r=load_ksong(fn);
          csynth=0;
          for(i=0;i<MAX_SYNTH;i++) cpatch[i]=0;
          audio_loadpatch(0, csynth, cpatch[csynth]);
          seq_render_start=0;
          seq_render_end=0;
          console_post("Song loaded successfully from disk!");
        }
        synth_releaseaudio();
      }
      // use this as the new song path
      dotfile_setvalue("songFileDir", (char*)&songfd[songfd_active].cpath);
      dotfile_save();
    }
    audiomode=AUDIOMODE_COMPOSING;
  }
}