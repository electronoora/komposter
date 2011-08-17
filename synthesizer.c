/*
 * Komposter (c) 2010-2011 Jani Halme (Firehawk/TDA)
 *
 * This code is licensed under the GNU General Public
 * License version 2. See file 'doc/LICENSE'.
 *
 * Synthesizer editor page
 *
 * $Rev$
 * $Date$
 */

#include "synthesizer.h"

// button indexes
#define B_PREV 0
#define B_NEXT 1
#define B_NAME 2
#define B_ADD  3
#define B_LOAD 4
#define B_SAVE 5
#define B_LABEL_INCSCALE 6
#define B_LABEL_DECSCALE 7
#define B_CLEAR 8

// file dialogs
#define FD_SAVE 0
#define FD_LOAD 1

#define MODULE_PALETTE 13

#define SQR(X) X*X

// from audio.c
extern int audio_spinlock;
extern int audiomode;

// from patch.c
extern int cpatch[MAX_SYNTH];

// state variables for module/signal drag'n'drop
int moduledrag=-1;
int signaldrag=-1;
float sigdrag_x, sigdrag_y;
int dragoffset_x, dragoffset_y;

// ui elements at the bottom
int synth_ui[9];
int kbfocus=-1;

// global data for synthesizer modules
synthmodule mod[MAX_SYNTH][MAX_MODULES];
int signalfifo[MAX_SYNTH][MAX_MODULES]; // module execution stack
char synthname[MAX_SYNTH][128];

int csynth; // currently active synth

// temp modules for the add module dialog
synthmodule tmpmod[MODULE_PALETTE];

// file dialog structs for load/save synth
filedialog fd[2];
int fd_active=-1;


// label editing dialog
int synth_label_edit;
char synth_label_text[65];
int synth_label_kbfocus;


// wait for audio spin lock to go down and mute the audio
void synth_lockaudio(void)
{
  // a potential race condition here - be careful. :)
  while (audio_spinlock) usleep(5);
  audiomode=AUDIOMODE_MUTE;
}


// resume the audio playback
void synth_releaseaudio(void)
{
  audiomode=AUDIOMODE_COMPOSING;
}


void synth_init(void)
{
 int s,m,n;

  // set synthesizers to defaults
  for(s=0;s<MAX_SYNTH;s++) {
    for(m=0;m<MAX_MODULES;m++) { mod[s][m].type=-1; mod[s][m].scale=0; }
    mod[s][0].type=MOD_CV;     mod[s][0].x=100; mod[s][0].y=80;
    mod[s][1].type=MOD_ACCENT; mod[s][1].x=200; mod[s][1].y=80; //mod[s][1].scale=SCALE_RAMP;
    mod[s][2].type=MOD_OUTPUT; mod[s][2].x=300; mod[s][2].y=80;
    strcpy((char*)(&synthname[s]), "Unnamed synthesizer");
    mod[s][m].outputpos=0;
  }

  // clear all patch cables
  for(s=0;s<MAX_SYNTH;s++)  
  {
    // clear all patch cables 
    for(m=0;m<MAX_MODULES;m++)
      for(n=0;n<4;n++) mod[s][m].input[n]=-1;

    // reset all active modules/nodes 
    resetactive(&mod[s][0], MAX_MODULES);
  }

  // set first synth visible
  csynth=0;

  // just to be sure when re-initializing
  synth_stackify(csynth);
  kmm_gcollect();
  
  // no dialogs visible
  fd_active=-1;
  strcpy((char*)(&fd[FD_SAVE].title), "save synthesizer");
  strcpy((char*)(&fd[FD_LOAD].title), "load synthesizer");  
  synth_label_edit=-1;
}



void synth_clear(int csyn)
{
  int m;

  for(m=0;m<MAX_MODULES;m++) {
    mod[csyn][m].type=-1;
    mod[csyn][m].scale=0;
    mod[csyn][m].input[0]=-1;
    mod[csyn][m].input[1]=-1;
    mod[csyn][m].input[2]=-1;
    mod[csyn][m].input[3]=-1;
  }
  mod[csyn][0].type=MOD_CV;     mod[csyn][0].x=100; mod[csyn][0].y=80;
  mod[csyn][1].type=MOD_ACCENT; mod[csyn][1].x=200; mod[csyn][1].y=80;
  mod[csyn][2].type=MOD_OUTPUT; mod[csyn][2].x=300; mod[csyn][2].y=80;
  strcpy((char*)(&synthname[csyn]), "Unnamed synthesizer");
  mod[csyn][m].outputpos=0;
  synth_stackify(csyn);
  kmm_gcollect();
}



// hovering above a module?
int hovertest_module(int x, int y, synthmodule *list, int count)
{
  int i,m=-1;

  for (i=0;i<count;i++) {
    if ( (list[i].type >= 0) &&
         (x>=(list[i].x - MODULE_HALF)) &&
         (x<=(list[i].x + MODULE_HALF)) &&
         (y>=(list[i].y - MODULE_HALF)) &&
         (y<=(list[i].y + MODULE_HALF))
       ) { m=i; }
  }
  return m;
}


// hovering near an output node? if not, return -1.
int hovertest_output(int x, int y, synthmodule *list, int count)
{
  int m;
  float xd, yd, d;

  for(m=0;m<count;m++) {
    if (list[m].type!=MOD_OUTPUT && list[m].type>=0) {
      xd=x-(list[m].x+MODULE_HALF);
      yd=y-(list[m].y+OUTPUT_OFFSET);
      d=sqrt(xd*xd+yd*yd);
      if (d<=NODE_PROXIMITY) return m;
    }
  }
  return -1;
}


// hovering near an input node of a module? if not, return -1.
int hovertest_input(int x, int y, synthmodule *mod)
{
  int n;
  float xd, yd, d;

  for(n=0;n<modInputCount[mod->type];n++) {
    if (mod->type<0) continue;
    xd=x - (mod->x + node_xoffset[ modInputCount[mod->type] ][ n ]);
    yd=y - (mod->y + node_yoffset[ modInputCount[mod->type] ][ n ]);
    d=sqrt(xd*xd + yd*yd);
    if (d<=NODE_PROXIMITY) return n;
  }
  return -1;
}


// reset all active modules and inputs
void resetactive(synthmodule *list, int count)
{
  int m;
  for(m=0;m<count;m++) { list[m].active=0; list[m].outactive=0; list[m].inpactive=-1; }
}


// get currently active module index or -1 if none
int getactive(synthmodule *list, int count)
{
  int m;
  for (m=0;m<count;m++) if (list[m].active) return m;
  return -1;
}


// get module index with currently active output node or -1 if none
int getactiveout(synthmodule *list, int count)
{
  int m;
  for (m=0;m<count;m++) if (list[m].outactive) return m;
  return -1;
}


// get module index with a currently active input node or -1 if none
int getactivein(synthmodule *list, int count)
{
  int m;
  for (m=0;m<count;m++) if (list[m].inpactive>=0) return m;
  return -1;
}


void synth_open_addmodule(void)
{
  int i;

  // fire up the add new module dialog
  for(i=1;i<14;i++) {
    tmpmod[i-1].type=i;
    tmpmod[i-1].active=0;
    tmpmod[i-1].outactive=0;
    tmpmod[i-1].inpactive=-1;
    if (i<8) {
      tmpmod[i-1].x=(DS_WIDTH/2)-222 + (i-1)*74;
      tmpmod[i-1].y=(DS_HEIGHT/2)-60;
    } else {
      if (i<16) {
        tmpmod[i-1].x=(DS_WIDTH/2)-222 + (i-8)*74;
        tmpmod[i-1].y=(DS_HEIGHT/2)+16;
      } else {
        // third row
      }
    }
  }        
  dialog_open(&synth_draw_addmodule, &synth_addmodule_hover, synth_addmodule_click);
  dialog_bindkeyboard(&synth_addmodule_keyboard);
}


//
// kb&mouse functions for synthesizer page
//
void synth_mouse_hover(int x, int y)
{
  int m,n;

  // no hover if currently dragging something
  if (moduledrag>=0 || signaldrag>=0) return;

  // reset all active flags from modules
  resetactive(&mod[csynth][0], MAX_MODULES);

  // test buttons
  synth_ui[B_PREV]=hovertest_box(x, y, 14,  DS_HEIGHT-14, 16, 16);
  synth_ui[B_NEXT]=hovertest_box(x, y, 64,  DS_HEIGHT-14, 16, 16);
  synth_ui[B_ADD]=hovertest_box (x, y, 310, DS_HEIGHT-14, 16, 16);
  synth_ui[B_SAVE]=hovertest_box(x, y, 350, DS_HEIGHT-14, 16, 16);
  synth_ui[B_LOAD]=hovertest_box(x, y, 372, DS_HEIGHT-14, 16, 16);
  synth_ui[B_CLEAR]=hovertest_box(x, y, 394, DS_HEIGHT-14, 16, 16) | (synth_ui[B_CLEAR]&8);

  // test focus on synth name textbox
  synth_ui[B_NAME]&=0x06;
  synth_ui[B_NAME]|=hovertest_box(x, y, 190, DS_HEIGHT-14, 16, 180);  

  // any ui-elements active?
  for(m=0;m<6;m++) if (synth_ui[m]&1) return;

  // hovering near an output node of a module?
  if ((m=hovertest_output(x, y, &mod[csynth][0], MAX_MODULES)) >= 0) {
    mod[csynth][m].outactive=1; return; }

  // hovering near a connected input node?
  for(m=0;m<MAX_MODULES;m++) {
    mod[csynth][m].inpactive=-1;
    if (mod[csynth][m].type>=0)
      if ((n=hovertest_input(x, y, &mod[csynth][m])) >= 0) {
        if (mod[csynth][m].input[n] >= 0) { mod[csynth][m].inpactive=n; return; }
    }
  }

  // hovering above a module?
  if ((m=hovertest_module(x, y, &mod[csynth][0], MAX_MODULES)) >= 0) mod[csynth][m].active=1;
}

void synth_mouse_drag(int x, int y)
{
  int m,n;

  if (signaldrag>=0) {
    sigdrag_x=(float)(x);
    sigdrag_y=(float)(y);

    // endpoint hovering near an input node?
    for(m=0;m<MAX_MODULES;m++) {
      mod[csynth][m].inpactive=-1;
      if ((n=hovertest_input(x, y, &mod[csynth][m])) >= 0)
        mod[csynth][m].inpactive=n;
    }
  }

  if (moduledrag>=0) {
    mod[csynth][moduledrag].x=x-dragoffset_x;
    mod[csynth][moduledrag].y=y-dragoffset_y;
    if (mod[csynth][moduledrag].y<32) mod[csynth][moduledrag].y=32;
    if (mod[csynth][moduledrag].y>DS_HEIGHT) mod[csynth][moduledrag].y=DS_HEIGHT;
    if (mod[csynth][moduledrag].x<0) mod[csynth][moduledrag].x=0;
    if (mod[csynth][moduledrag].x>DS_WIDTH) mod[csynth][moduledrag].x=DS_WIDTH;
  }
}

void synth_mouse_click(int button, int state, int x, int y)
{
  int m,i;
  char tmps[256], *syndir;

  if (button==GLUT_LEFT_BUTTON) {
    if (state==GLUT_DOWN) {

      // test first if modifiers are pressed
      m=glutGetModifiers();
      if (m==GLUT_ACTIVE_SHIFT) {
        if ((m=getactive(&mod[csynth][0],MAX_MODULES)) >=0 ) {
          // todo: maybe a confirmation first?
          synth_lockaudio();
          if (m>2) synth_deletemodule(m);
          synth_stackify(csynth); // re-arrange signal stack
          synth_releaseaudio();
        }
        return;
      }
 
      // remove kb focus it it was assigned
      if (kbfocus>=0) { synth_ui[kbfocus]&=0x03; kbfocus=-1; glutIgnoreKeyRepeat(1); }

      if (synth_ui[B_CLEAR]&1) {
        if (synth_ui[B_CLEAR]&8) {
          synth_lockaudio();
          synth_clear(csynth);
          synth_releaseaudio();
          sprintf(tmps, "Synthesizer %02d cleared and reset to defaults", csynth);
          console_post(tmps);
          synth_ui[B_CLEAR]&=0xff-8;
        } else {
          console_post("Click again to clear synthesizer");
          synth_ui[B_CLEAR]|=8;
        }
        return;
      } else {
        synth_ui[B_CLEAR]&=0xff-8;
      }
    
      // test clicks to ui elements
      if (synth_ui[B_PREV]) { // select previous synth to voice 0
        if (csynth>0) {
          synth_lockaudio();
          csynth--;
          synth_stackify(csynth);
          kmm_gcollect();
          synth_releaseaudio();
          return;
        }
      }
      if (synth_ui[B_NEXT]) { 
        if (csynth<(MAX_SYNTH-1)) {
          synth_lockaudio();
          csynth++; 
          synth_stackify(csynth); 
          kmm_gcollect(); 
          synth_releaseaudio();
          return; 
        }
      }

      if (synth_ui[B_NAME]) {
        // set keyboard focus
        synth_ui[B_NAME]|=4;
        kbfocus=B_NAME;
        glutIgnoreKeyRepeat(0);
        return;
      }
      if (synth_ui[B_ADD]) {
        synth_open_addmodule();
        return;
      }
      
      if (synth_ui[B_SAVE]) {
        syndir=dotfile_getvalue("synthFileDir");      
        filedialog_open(&fd[FD_SAVE], "ksyn", syndir);        
        fd_active=FD_SAVE;
        dialog_open(&synth_draw_file, &synth_file_hover, &synth_file_click);
        dialog_bindkeyboard(&synth_file_keyboard);
        dialog_binddrag(&synth_file_drag);
        return;
      }
      if (synth_ui[B_LOAD]) {
        syndir=dotfile_getvalue("synthFileDir");
        filedialog_open(&fd[FD_LOAD], "ksyn", syndir);        
        fd_active=FD_LOAD;
        dialog_open(&synth_draw_file, &synth_file_hover, &synth_file_click);
        dialog_bindkeyboard(&synth_file_keyboard);        
        dialog_binddrag(&synth_file_drag);
        return;
      }

    
      // output nodes active?
      if ((m=getactiveout(&mod[csynth][0],MAX_MODULES)) >=0 ) {
        signaldrag=m;
        sigdrag_x=(float)(x);
        sigdrag_y=(float)(y);
      }

      // input nodes active?
      if ((m=getactivein(&mod[csynth][0],MAX_MODULES)) >=0 ) {
        // ok, detach the patch from the input and drag the patch
        signaldrag=mod[csynth][m].input[mod[csynth][m].inpactive];
        mod[csynth][m].input[mod[csynth][m].inpactive]=-1;
        sigdrag_x=(float)(x);
        sigdrag_y=(float)(y);
      }

      // modules active?
      if ((m=getactive(&mod[csynth][0],MAX_MODULES)) >=0 ) {
        moduledrag=m;
        dragoffset_x=x-mod[csynth][m].x;
        dragoffset_y=y-mod[csynth][m].y;
      }
    }
    if (state==GLUT_UP) {
      if (moduledrag>=0) { moduledrag=-1; }
      if (signaldrag>=0) {
        // make a patch between the modules if dropped to an input
        if ((m=getactivein(&mod[csynth][0],MAX_MODULES)) >=0 ) {
          synth_lockaudio();
          // make a patch
          mod[csynth][m].input[mod[csynth][m].inpactive]=signaldrag;
          sprintf(tmps, "Signal from %s patched to %s(%s)!\n",
            modTypeNames[mod[csynth][signaldrag].type],
            modTypeNames[mod[csynth][m].type],
            modInputNames[mod[csynth][m].type][mod[csynth][m].inpactive]
          );
          console_post(tmps);
          // change the label of a cv module to match the patch target
          if (mod[csynth][signaldrag].type==MOD_KNOB) {
            snprintf((char*)(&mod[csynth][signaldrag].label), 64,
              "%s", modInputNames[mod[csynth][m].type][mod[csynth][m].inpactive]
            );
          }
          synth_stackify(csynth); // re-arrange signal stack for new route
          synth_releaseaudio();
        }
        signaldrag=-1;
        // re-set active flags
        synth_mouse_hover(x,y);
      }
    }
  }

  if (button==GLUT_RIGHT_BUTTON && state==GLUT_DOWN) {
    m=hovertest_module(x,y,&mod[csynth][0],MAX_MODULES);
    if (m>=0) {
      if(mod[csynth][m].type==MOD_KNOB || mod[csynth][m].type==MOD_ATTENUATOR) {
        // pop up for editing the label
        synth_label_edit=m;
        strncpy((char*)(&synth_label_text), mod[csynth][m].label, 64);
        dialog_open(&synthlabel_draw, &synthlabel_hover, &synthlabel_click);
        dialog_bindkeyboard(&synthlabel_keyboard);   
        return;
      } else {
        sprintf(tmps, "This module type has no editable parameters");
        console_post(tmps);
      }
    }
  }
}

// keyboard callback
void synth_keyboard(unsigned char key, int x, int y)
{
  char *syndir;

  if (kbfocus==B_NAME) {
    // edit synth name
    if (key==13) { synth_ui[B_NAME]&=0x03; kbfocus=-1; glutIgnoreKeyRepeat(1); }
    textbox_edit(synthname[csynth], key, 19);
    return;
  }

  if (key=='s' || key=='S') {
//    strcpy((char*)(&fd[FD_SAVE].fname), "newfilename");
    syndir=dotfile_getvalue("synthFileDir");
    filedialog_open(&fd[FD_SAVE], "ksyn", syndir);        
    fd_active=FD_SAVE;
    dialog_open(&synth_draw_file, &synth_file_hover, &synth_file_click);
    dialog_bindkeyboard(&synth_file_keyboard);
    dialog_binddrag(&synth_file_drag);
    return;
  }
  if (key=='l' || key=='L') {
//    strcpy((char*)(&fd[FD_LOAD].fname), "test");
    syndir=dotfile_getvalue("synthFileDir");    
    filedialog_open(&fd[FD_LOAD], "ksyn", syndir);        
    fd_active=FD_LOAD;
    dialog_open(&synth_draw_file, &synth_file_hover, &synth_file_click);
    dialog_bindkeyboard(&synth_file_keyboard);        
    dialog_binddrag(&synth_file_drag);
    return;
  }
  if (key=='m' || key=='M') {
    synth_open_addmodule();
    return;
  }
}  



void synth_specialkey(unsigned char key, int x, int y)
{
  switch(key)
  {
    case GLUT_KEY_LEFT:
      if (csynth>0) { csynth--; synth_stackify(csynth); }
      break;
      
    case GLUT_KEY_RIGHT:
      if (csynth<(MAX_SYNTH-1)) { csynth++; synth_stackify(csynth); }
      break;
  }
}



//
// draw functions for synthesizer page
//
void synth_draw(void)
{
  int m,n;
  char tmps[16];
  float x1,y1,x2,y2;
  float cx1,cy1,cx2,cy2;
  
  // draw patch "cables"
  for(m=0;m<MAX_MODULES;m++) {
    if (mod[csynth][m].type>=0)
      for(n=0;n<modInputCount[mod[csynth][m].type];n++) {
        if (mod[csynth][m].input[n]>=0) {
          // output node position
          x1=mod[csynth][mod[csynth][m].input[n]].x+MODULE_HALF;
          y1=mod[csynth][mod[csynth][m].input[n]].y+OUTPUT_OFFSET;
          cx1=x1+MODULE_HALF; cy1=y1;
          
          // input node position
          x2=mod[csynth][m].x + node_xoffset[modInputCount[mod[csynth][m].type]][n];
          y2=mod[csynth][m].y + node_yoffset[modInputCount[mod[csynth][m].type]][n];
          cx2=x2; cy2=y2;
          if (abs(node_xoffset[modInputCount[mod[csynth][m].type]][n]) > abs(node_yoffset[modInputCount[mod[csynth][m].type]][n])) {
            cx2+=node_xoffset[modInputCount[mod[csynth][m].type]][n];
          } else {
            cy2+=node_yoffset[modInputCount[mod[csynth][m].type]][n];
          }
          draw_patch_control(x1, y1, x2, y2, cx1, cy1, cx2, cy2);
        }
      }
  }

  // draw modules
  for(m=0;m<MAX_MODULES;m++) {
    if (mod[csynth][m].type>=0) {
      draw_module(&mod[csynth][m]);
      sprintf(tmps, "%02d", m);
      render_text(tmps, mod[csynth][m].x+MODULE_HALF+11, mod[csynth][m].y+MODULE_HALF, 2, 0xff505050, 1);
    }
   }
        
  // draw patch being dragged
  if (signaldrag>=0)
    draw_patch(mod[csynth][signaldrag].x+MODULE_HALF, mod[csynth][signaldrag].y+OUTPUT_OFFSET, sigdrag_x, sigdrag_y);
    
  // draw interface elements for synth page
  draw_button(14, DS_HEIGHT-14, 16, "<<", synth_ui[B_PREV]);    
  sprintf(tmps, "%02d", csynth);
  draw_textbox(39, DS_HEIGHT-14, 16, 24, tmps, 0);
  draw_button(64, DS_HEIGHT-14, 16, ">>", synth_ui[B_NEXT]);
  draw_textbox(190, DS_HEIGHT-14, 16, 180, synthname[csynth], synth_ui[B_NAME]);
  draw_button(310, DS_HEIGHT-14, 16, "m", synth_ui[B_ADD]);
  draw_button(350, DS_HEIGHT-14, 16, "S", synth_ui[B_SAVE]);
  draw_button(372, DS_HEIGHT-14, 16, "L", synth_ui[B_LOAD]);

  draw_button(394, DS_HEIGHT-14, 16, "C", synth_ui[B_CLEAR]);
}

//
// dialog and mouse functions for adding a new module
//
void synth_addmodule(int type)
{
  int m,n;
  int i, j, k, mi, mj, bmi, bmj;
  float d, da, mda;
  float xd, yd;
  char tmps[256];

  for (m=0;m<MAX_MODULES;m++) {
    if (mod[csynth][m].type==-1) {
      mod[csynth][m].type=type;
      for(n=0;n<4;n++) mod[csynth][m].input[n]=-1;
      if (type==MOD_KNOB || type==MOD_ATTENUATOR) strncpy(mod[csynth][m].label, modTypeNames[type], 64);

      // find the best position to place the new module into
      bmi=100; bmj=100;
      mda=0;
      for(j=1;j<9;j++) {
        for(i=1;i<9;i++) {
          da=10000;
          for(k=0;k<MAX_MODULES;k++) {
            if (mod[csynth][k].type>=0 && k!=m) {
              xd=(DS_WIDTH/10)*i - mod[csynth][k].x;
              yd=(DS_HEIGHT/10)*j - mod[csynth][k].y;
              d=sqrt(xd*xd+yd*yd);
              if (d<da) { mi=(DS_WIDTH/10)*i; mj=(DS_HEIGHT/10)*j; da=d; }
            }
          }
          if (da>mda) { mda=da; bmi=mi; bmj=mj; }
        }
      }
      mod[csynth][m].x=bmi;
      mod[csynth][m].y=bmj;

      kmm_gcollect();
      
      sprintf(tmps, "Added module %s", modTypeNames[type]);
      console_post(tmps);
      return;
    }
  }
}

void synth_deletemodule(int m)
{
  int i,j;

  // retain the index number of all modules after this
  mod[csynth][m].type=-1;
  mod[csynth][m].input[0]=-1; mod[csynth][m].input[1]=-1;
  mod[csynth][m].input[2]=-1; mod[csynth][m].input[3]=-1;
  mod[csynth][m].active=0;
  mod[csynth][m].outactive=0;  mod[csynth][m].inpactive=0;
  strncpy(mod[csynth][m].label, "", 64);
  for (i=0;i<MAX_MODULES;i++)
    for(j=0;j<4;j++)
      if (mod[csynth][i].input[j]==m) mod[csynth][i].input[j]=-1;
  kmm_gcollect();
}

void synth_draw_addmodule(void)
{
  int i,m;

  draw_textbox((DS_WIDTH/2), (DS_HEIGHT/2), 260, 526, "", 0);

  render_text("Add new module:", (DS_WIDTH/2)-254, (DS_HEIGHT/2)-108, 0, 0xffb05500, 0);

  for(i=0;i<MODULE_PALETTE;i++) draw_module(&tmpmod[i]);
  m=getactive(&tmpmod[0], MODULE_PALETTE);
  if (m>=0) {
    render_text(modTypeDescriptions[tmpmod[m].type], (DS_WIDTH/2)-254, (DS_HEIGHT/2)+75, 3, 0xffe0e0e0, 0);
  }

  render_text("esc/right click to close", (DS_WIDTH/2)+261, (DS_HEIGHT/2)+127, 2, 0xffc0c0c0, 2);
}
void synth_addmodule_hover(int x, int y)
{
  int m;
  
  resetactive(&tmpmod[0], MODULE_PALETTE);
  if ((m=hovertest_module(x, y, &tmpmod[0], MODULE_PALETTE)) >= 0) {
    tmpmod[m].active=1;
  }
  
}
void synth_addmodule_click(int button, int state, int x, int y)
{
  int m;
  
  if (button==GLUT_RIGHT_BUTTON && hovertest_box(x,y,(DS_WIDTH/2),(DS_HEIGHT/2),260,526)) {
    for(m=0;m<6;m++) synth_ui[m]&=0x06;
    dialog_close(); return; }
  if (button==GLUT_LEFT_BUTTON) {
    m=getactive(&tmpmod[0], MODULE_PALETTE);
    if (m>=0) {
      synth_addmodule(tmpmod[m].type);
      dialog_close();
      return;
    }
  }
}

void synth_addmodule_keyboard(unsigned char key, int x, int y)
{
  if (key==27) { dialog_close(); return; }
}



//
// file dialog functions
//
void synth_draw_file(void)
{
  filedialog_draw(&fd[fd_active]);
}
void synth_file_hover(int x, int y)
{
  filedialog_hover(&fd[fd_active], x, y);
}
void synth_file_click(int button, int state, int x, int y)
{
  filedialog_click(&fd[fd_active], button, state, x, y);
  synth_file_checkstate();
}
void synth_file_keyboard(unsigned char key, int x, int y)
{
  filedialog_keyboard(&fd[fd_active], key);
  synth_file_checkstate();
}
void synth_file_drag(int x, int y)
{
  filedialog_drag(&fd[fd_active], x, y);
}
void synth_file_checkstate(void)
{
  FILE *f;
  int r;
  char fn[255], tmps[255];;

  if (fd[fd_active].exitstate) {
    dialog_close();
    if (fd[fd_active].exitstate==FDEXIT_OK) {
      // clicked ok, do stuff with the filename
      sprintf(fn, "%s", fd[fd_active].fullpath);
      if (fd_active==FD_SAVE) {
        f=fopen(fn, "w");
        if (f) {
          r=save_chunk_ksyn(csynth, f);
          if (!r) {
            save_chunk_kbnk(csynth, f);
            if (r) { console_post("Writing to file failed (KBNK chunk)");
            } else {
              sprintf(tmps, "Synthesizer %s saved to disk as %s", synthname[csynth], fn);
              console_post(tmps);
            }
          } else {
            console_post("Writing to file failed (KSYN chunk)");
          }
          fclose(f);
        } else {
          console_post("Unable to open file for writing!");
        }
      }
      if (fd_active==FD_LOAD) {
        synth_lockaudio();
        f=fopen(fn, "r");
        if (f) {
          synth_clear(csynth);
          synth_stackify(csynth);
          r=load_chunk_ksyn(csynth, f);
          if (!r) {
            r=load_chunk_kbnk(csynth, f);
            if (!r) {
              synth_stackify(csynth);
              audio_loadpatch(0, csynth, cpatch[csynth]);              
              sprintf(tmps, "Synthesizer %s loaded from disk to slot %02d", synthname[csynth], csynth);
              console_post(tmps);
            } else {
              console_post("Reading file failed (KBNK chunk)");
            }
          } else {
            console_post("Readinf file failed (KSYN chunk)");
          }
          fclose(f);
        } else {
          console_post("Unable to open file for reading!");
        }
        kmm_gcollect();
        synth_releaseaudio();
      }
      // use this as the new synth path
      dotfile_setvalue("synthFileDir", (char*)&fd[fd_active].cpath);
      dotfile_save();
    }
  } 
}



//
// module label edit dialog
//
void synthlabel_draw(void)
{
  char tmps[128];

  draw_textbox((DS_WIDTH/2), (DS_HEIGHT/2), 120, 280, "", 0);
  
  sprintf(tmps,"%s (%02d)",  modTypeNames[mod[csynth][synth_label_edit].type], synth_label_edit);
  render_text(tmps, (DS_WIDTH/2)-128, (DS_HEIGHT/2)-40, 0, 0xffb05500, 0);
  render_text("esc/right click to close", (DS_WIDTH/2)+138, (DS_HEIGHT/2)+58, 2, 0xffc0c0c0, 2);

//  sprintf(tmps, "%02d: %s", seq_synth[seq_chlabel_hover], synthname[seq_synth[seq_chlabel_hover]]);
  render_text("label:", (DS_WIDTH/2)-120, (DS_HEIGHT/2)-13, 2, 0xffc0c0c0, 0);
  draw_textbox((DS_WIDTH/2)+30, (DS_HEIGHT/2)-16, 16, 180, synth_label_text, synth_label_kbfocus);

  render_text("scale:", (DS_WIDTH/2)-120, (DS_HEIGHT/2)+21, 2, 0xffc0c0c0, 0);
  draw_button((DS_WIDTH/2)+112, (DS_HEIGHT/2)+18, 16, ">>", synth_ui[B_LABEL_INCSCALE]);
  draw_button((DS_WIDTH/2)-52,  (DS_HEIGHT/2)+18, 16, "<<", synth_ui[B_LABEL_DECSCALE]);
  draw_textbox((DS_WIDTH/2)+30, (DS_HEIGHT/2)+18, 16, 140, knobScaleNames[mod[csynth][synth_label_edit].scale], 0);
}


void synthlabel_hover(int x, int y)
{
  synth_label_kbfocus&=0x06;
  synth_label_kbfocus|=hovertest_box(x, y, (DS_WIDTH/2)+30, (DS_HEIGHT/2)-16, 16, 180);

  synth_ui[B_LABEL_INCSCALE]=hovertest_box(x, y, (DS_WIDTH/2)+112, (DS_HEIGHT/2)+18, 16, 16);
  synth_ui[B_LABEL_DECSCALE]=hovertest_box(x, y, (DS_WIDTH/2)-52,  (DS_HEIGHT/2)+18, 16, 16);
}


void synthlabel_click(int button, int state, int x, int y)
{
  int i,j;

  if (button==GLUT_LEFT_BUTTON) {
    if (state==GLUT_DOWN) {
      if (synth_label_kbfocus) {
        // give focus
        synth_label_kbfocus|=4;
        glutIgnoreKeyRepeat(0);
      } else { synth_label_kbfocus&=0x03; glutIgnoreKeyRepeat(1); }
      
      if (synth_ui[B_LABEL_INCSCALE]) {
        if (mod[csynth][synth_label_edit].scale<(KNOBSCALES-1)) mod[csynth][synth_label_edit].scale++;
      }
      if (synth_ui[B_LABEL_DECSCALE])
        if (mod[csynth][synth_label_edit].scale>0) mod[csynth][synth_label_edit].scale--;
      
    }
  }
  if (button==GLUT_RIGHT_BUTTON && state==GLUT_DOWN && hovertest_box(x,y,(DS_WIDTH/2),(DS_HEIGHT/2),150,240 )) {
    strncpy(mod[csynth][synth_label_edit].label, synth_label_text, 64);
    synth_label_edit=-1; 
    synth_label_kbfocus=0;
    glutIgnoreKeyRepeat(1);
    dialog_close();
    return; 
  }
}


void synthlabel_keyboard(unsigned char key, int x, int y)
{
  if (synth_label_kbfocus&0x04) {
    // edit synth name
    if (key==13) { synth_label_kbfocus&=0x03; kbfocus=-1; glutIgnoreKeyRepeat(1); }
    textbox_edit(synth_label_text, key, 16);
    return;
  }
  if (key==13 || key==27) { // enter or esc
    strncpy(mod[csynth][synth_label_edit].label, synth_label_text, 64);
    synth_label_edit=-1; 
    synth_label_kbfocus=0;
    glutIgnoreKeyRepeat(1);
    dialog_close();
    return;     
  }
}    
