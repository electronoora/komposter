/*
 * Komposter (c) 2010-2011 Jani Halme (Firehawk/TDA)
 *
 * This code is licensed under the GNU General Public
 * License version 2. See file 'doc/LICENSE'.
 *
 * Patch editor page
 *
 * $Rev$
 * $Date$
 */

#include "patch.h"

// button indexes
#define B_PREV 0
#define B_NEXT 1
#define B_PREVSYN 2
#define B_SYNNAME 3
#define B_NEXTSYN 4
#define B_PATCHNAME 5

#define B_MOD_ADDPREC 0
#define B_MOD_DECPREC 1
#define B_MOD_VALUE   2

#define PIANO_LEFT	8
#define PIANO_WIDTH	(9*16*7)
#define PIANO_RIGHT     (PIANO_LEFT+PIANO_WIDTH)
#define PIANO_TOP	520
#define PIANO_HEIGHT	70
#define PIANO_BOTTOM	(PIANO_TOP+PIANO_HEIGHT+2)

#define PATCHES_PER_COLUMN	30

extern int csynth;
extern char synthname[MAX_SYNTH][128]; // from synthesizer.c
extern synthmodule mod[MAX_SYNTH][MAX_MODULES]; // ditto
extern int signalfifo[MAX_SYNTH][MAX_MODULES]; // module execution stack

// from modules.c
extern float pitch[MAX_SYNTH];
extern int accent[MAX_SYNTH];
extern int gate[MAX_SYNTH];

// from pattern.c
extern int coct;

// from sequencer.c
extern int bpm;

// keyboard keys to mimic piano keys
char pianokeys[36]={
  'z', 's', 'x', 'd', 'c', 'v', 'g', 'b', 'h', 'n', 'j', 'm', // octave 1
  'q', '2', 'w', '3', 'e', 'r', '5', 't', '6', 'y', '7', 'u', // octave 2
  'i', '9', 'o', '0', 'p',  0,   0,   0,   0,   0,   0,   0}; // partial octave 3, null terminates



int patch_ui[6];
int modulator_ui[3];
int cpkey=-1;
int cphover=-1;
int cpkeydown=-1;
int kpkeydown=-1;
int patchkbfocus=0;
int modkbfocus=0;
char modeditbox[128];
int patchcursor=0;

int cpatch[MAX_SYNTH]; // selected patch for each synth
char patchname[MAX_SYNTH][MAX_PATCHES][128];
float modvalue[MAX_SYNTH][MAX_PATCHES][MAX_MODULES];
int modquantifier[MAX_SYNTH][MAX_PATCHES][MAX_MODULES];

float patch_modulator_floatval;
int patch_modulator_intval;


void patch_init()
{
  int i, j, k;
  
  cphover=-1; cpkey=-1; cpkeydown=-1;
  patchkbfocus=0;
  for(i=0;i<6;i++) patch_ui[i]=0;

  for(i=0;i<MAX_SYNTH;i++) {
    cpatch[i]=0;
    for(j=0;j<MAX_PATCHES;j++) {
      strcpy((char*)(&patchname[i][j]), "Unnamed patch");
      for(k=0;k<MAX_MODULES;k++) {
        modvalue[i][j][k]=0.0f;
        modquantifier[i][j][k]=32;
      }
    }
  }

  synth_stackify(csynth);
}


void patch_mouse_hover(int x, int y)
{
  int m;
  int octave, key, xoct;
  int wk[7]={0,2,4,5,7, 9,11};
  int bk[7]={1,3,0,6,8,10, 0};
  int pk[7]={0,1,3,0,6, 8,10};

  // test buttons
  patch_ui[B_PREV]=hovertest_box(x, y, 310,  DS_HEIGHT-14, 16, 16);
  patch_ui[B_NEXT]=hovertest_box(x, y, 362,  DS_HEIGHT-14, 16, 16);
  patch_ui[B_PREVSYN]=hovertest_box(x, y, 14, DS_HEIGHT-14, 16, 16);
  patch_ui[B_NEXTSYN]=hovertest_box(x, y, 230, DS_HEIGHT-14, 16, 16);

  // test patch name box
  patch_ui[B_PATCHNAME]&=0x06;
  patch_ui[B_PATCHNAME]|=hovertest_box(x, y, 472, DS_HEIGHT-14, 16, 180);  

  // any ui-elements active?
  for(m=0;m<6;m++) if (patch_ui[m]&1) return;

  // test modulators
  cphover=-1;
  if (x>=20 && x<380 && y>=8 && y<(8+PATCHES_PER_COLUMN*16)) {
    cphover=((y-8)/16);
  }
  if (x>=400 && x<780 && y>=8 && y<(8+PATCHES_PER_COLUMN*16)) {
    cphover=(PATCHES_PER_COLUMN + (y-8)/16);
  }

  // test piano keyboard
  cpkey=-1;
  if (x>=PIANO_LEFT && x<PIANO_RIGHT && y>=PIANO_TOP && y<PIANO_BOTTOM) {

    if (y>(PIANO_TOP + PIANO_HEIGHT/2)) { // white keys only
      octave=(x-PIANO_LEFT)/(7*16);
      key=((x-PIANO_LEFT) % (7*16))/16;
      cpkey=octave*12 + wk[key];
    } else { // white and black
      octave=(x-PIANO_LEFT)/(7*16);

      // white key as default
      xoct=((x-PIANO_LEFT)%(7*16));
      key=xoct/16;
      cpkey=octave*12 + wk[key];
      switch (key) {
        // black key only to right:
        case 0:
        case 3:
          if ( (xoct-(key*16))>10 ) cpkey=octave*12 + bk[key];
          break;
      
        // black to right and left
        case 1:
        case 4:
        case 5:
          if ( (xoct-(key*16))>10) cpkey=octave*12 + bk[key];
          if ( (xoct-(key*16))<6 ) cpkey=octave*12 + pk[key];
          break;
          
        // black key to left
        case 2:
        case 6:
          if ( (xoct-(key*16))<6 ) cpkey=octave*12 + pk[key];
          break;
        
      }
    }
  }

}

void patch_mouse_drag(int x, int y)
{
}

void patch_mouse_click(int button, int state, int x, int y)
{
  int mi,mt,t;

  if (button==GLUT_LEFT_BUTTON) {
    if (state==GLUT_DOWN) {

      // remove kb focus it it was assigned
      if (patchkbfocus>=0) { patch_ui[patchkbfocus]&=0x03; patchkbfocus=-1; glutIgnoreKeyRepeat(1); }

      // click on the ui buttons
      if (patch_ui[B_PREV]) {
        if (cpatch[csynth]>0) cpatch[csynth]--; return;
        audio_loadpatch(0, csynth, cpatch[csynth]);
      }
      if (patch_ui[B_NEXT]) { if (cpatch[csynth]<MAX_PATCHES) cpatch[csynth]++; 
        audio_loadpatch(0, csynth, cpatch[csynth]);       
        return; }
      if (patch_ui[B_PREVSYN]) { if (csynth>0) { 
          synth_lockaudio();
          csynth--; 
          synth_stackify(csynth);
          kmm_gcollect();
          synth_releaseaudio();
          }
          audio_loadpatch(0, csynth, cpatch[csynth]);      
        return; }
      if (patch_ui[B_NEXTSYN]) { if (csynth<(MAX_SYNTH-1)) { 
          synth_lockaudio();
          csynth++; 
          synth_stackify(csynth); 
          kmm_gcollect();
          synth_releaseaudio();
        }
        audio_loadpatch(0, csynth, cpatch[csynth]);      
        return; }

      if (patch_ui[B_PATCHNAME]) {
        // set keyboard focus
        patch_ui[B_PATCHNAME]|=4;
        patchkbfocus=B_PATCHNAME;
        glutIgnoreKeyRepeat(0);
        return;
      }

      // click on piano
      if (cpkey>=0) { // && kpkeydown<0) {
        cpkeydown=cpkey;
        audio_trignote(0, cpkey);
      }

      if (cphover>=0 && signalfifo[csynth][cphover]>=0) {
        // clicked a modulator, spring up a settings dialog
        mi=signalfifo[csynth][cphover];
        mt=mod[csynth][mi].type;
        if (modModulatorTypes[mt]>0) {
          switch(mt) {
            case MOD_WAVEFORM:
              t=(int)(modvalue[csynth][cpatch[csynth]][mi]);
              t++; t%=VCO_WAVEFORMS;
              modvalue[csynth][cpatch[csynth]][mi]=(float)(t);
              break;
            case MOD_LFO:
              t=(int)(modvalue[csynth][cpatch[csynth]][mi]);
//              t++; t%=LFO_WAVEFORMS;
              t= (t==2)?3:2;
              modvalue[csynth][cpatch[csynth]][mi]=(float)(t);            
              break;
            case MOD_FILTER:
              t=(int)(modvalue[csynth][cpatch[csynth]][mi]);
              t++; t%=VCF_MODES;
              modvalue[csynth][cpatch[csynth]][mi]=(float)(t);
              break;
            case MOD_SWITCH:
              t=(int)(modvalue[csynth][cpatch[csynth]][mi]);
              t++; t%=4;
              modvalue[csynth][cpatch[csynth]][mi]=(float)(t);
              break;
            case MOD_DELAY:
              t=(int)(modvalue[csynth][cpatch[csynth]][mi]);
              t++; t%=2;
              modvalue[csynth][cpatch[csynth]][mi]=(float)(t);
              break;
            default:
              switch(modModulatorTypes[mod[csynth][mi].type]) {
                case 1: // float
                  patch_modulator_floatval=knob_float2scale(mod[csynth][mi].scale, modvalue[ csynth ][cpatch[csynth]][ mi ]);
                  sprintf(modeditbox, "%g", patch_modulator_floatval);
//                    knob_float2scale(mod[csynth][mi].scale, modvalue[ csynth ][cpatch[csynth]][ mi ]));
                  break;
                case 2: // integer
                  patch_modulator_intval=modvalue[ csynth ][cpatch[csynth]][ mi ];
                  break;
              }
              dialog_open(&patch_draw_modulator, &patch_modulator_hover, &patch_modulator_click);
              dialog_bindkeyboard(&patch_modulator_keyboard);
              dialog_bindspecial(&patch_modulator_special);
              break;
          }
          return;
        } else {
          console_post("No settings for this module!");
        } 
      }

    }
    if (state==GLUT_UP) {
      if (cpkeydown>=0) // && kpkeydown<0)
        { cpkeydown=-1; gate[0]=0; }

      patch_mouse_hover(x, y);
    } 
  }            
  // todo: rightclicks      
}

// keyboard callback
void patch_keyboard(unsigned char key, int x, int y)
{
  int i, pkey;
  
  if (patchkbfocus==B_PATCHNAME) {
    // edit patch name
    if (key==13) { patch_ui[B_PATCHNAME]&=0x03; patchkbfocus=-1; glutIgnoreKeyRepeat(1); }
    textbox_edit(patchname[csynth][cpatch[csynth]], key, 25);
    return;
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

void patch_keyboardup(unsigned char key, int x, int y)
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


void patch_specialkey(unsigned char key, int x, int y)
{
  switch(key)
  {
    case GLUT_KEY_LEFT:
      if (cpatch[csynth]>0) cpatch[csynth]--;
      break;
      
    case GLUT_KEY_RIGHT:
      if (cpatch[csynth]<MAX_PATCHES) cpatch[csynth]++;
      break;

    case GLUT_KEY_UP:
      if (csynth>0) { csynth--; synth_stackify(csynth); }
      break;
      
    case GLUT_KEY_DOWN:
      if (csynth<(MAX_SYNTH-1)) { csynth++; synth_stackify(csynth); }
      break;
  }
}



void patch_draw(void)
{
  int m, mm, mi, mt;
  char tmps[256], quant[256];
  int x, yd;
  int rkdown;
  float f;

  // run through the synth fifo and print out a list of modulators and their current values
  m=0; mm=0;
  x=0; yd=0;
  while(signalfifo[csynth][m]>=0) {
    mi=signalfifo[csynth][m];
    mt=mod[csynth][mi].type;

    if (mm>=PATCHES_PER_COLUMN) { x=400; yd=PATCHES_PER_COLUMN*16; }
    if (m==cphover) {
      glColor4f(1.0, 1.0, 1.0, 0.16);
      glBegin(GL_QUADS);
      glVertex2f(x+16,  8+m*16-yd);
      glVertex2f(x+380, 8+m*16-yd);
      glVertex2f(x+380, 24+m*16-yd);
      glVertex2f(x+16,  24+m*16-yd);
      glEnd();
    }

    sprintf(tmps, "%02d", mi); render_text(tmps, x+20, 20+mm*16-yd, 2, 0xff505050, 0);
    sprintf(tmps, "%s", modTypeNames[mod[csynth][mi].type]); render_text(tmps, x+44, 20+mm*16-yd, 2, 0xffc0c0c0, 0);

    quant[0]='\0';
    if (modModulatorTypes[mod[csynth][mi].type]==1)
      sprintf(quant, "(%d bit)",  modquantifier[csynth][ cpatch[csynth] ][ mi ]);
    sprintf(tmps, "%s %s", modModulatorNames[mod[csynth][mi].type], quant);
    if (mod[csynth][mi].type==MOD_KNOB || 
        mod[csynth][mi].type==MOD_ATTENUATOR) sprintf(tmps, "%s %s", mod[csynth][mi].label, quant);
    render_text(tmps, x+110, 20+mm*16-yd, 2, 0xff808080, 0);

    switch(modModulatorTypes[mod[csynth][mi].type])
    {
      case 0: tmps[0]='\0'; break; // no modulator value
      case 1: // float
        if (mt==MOD_KNOB || mt==MOD_ATTENUATOR || mt==MOD_ACCENT) {
          f=knob_float2scale(mod[csynth][mi].scale, modvalue[ csynth ][ cpatch[csynth] ][ mi ]);
          sprintf(tmps, "%g %s", f, knobScaleUnits[ mod[csynth][mi].scale ]);
          break;
        } else { sprintf(tmps, "%g", modvalue[ csynth ][ cpatch[csynth] ][ mi ]); break; }
      case 2: sprintf(tmps, "0x%08x", (int)(modvalue[ csynth ][ cpatch[csynth] ][ mi ])); break; // hex integer
      case 3: sprintf(tmps, "%s", modVcoWaveforms[(int)(modvalue[csynth][cpatch[csynth]][mi])]);break; // vco waveform bits
      case 4: sprintf(tmps, "%s", modLfoWaveforms[(int)(modvalue[csynth][cpatch[csynth]][mi])]);break; // lfo waveform bits
      case 5: sprintf(tmps, "%s", modVcfModes[(int)(modvalue[csynth][cpatch[csynth]][mi])]);break; // vcf mode
      case 6: sprintf(tmps, "%s", modDelayModes[(int)(modvalue[csynth][cpatch[csynth]][mi])]);break; // delay mode
    }
    if (mt==MOD_CV) sprintf(tmps, "%f hz", pitch[0]);
    render_text(tmps, x+250, 20+mm*16-yd, 2, 0xffc0c0c0, 0);
    m++; mm++;
  }
  
  // draw the piano keyboard for jazzing with the patch :)
  rkdown=-1;
  if (cpkeydown>=0) rkdown=cpkeydown;
  if (kpkeydown>=0) rkdown=kpkeydown;
  for(m=0;m<9;m++) {
    if ( m >= coct && m < (coct+3)) {
      draw_kbhoct(PIANO_LEFT+m*16*7, PIANO_TOP, 16, 70, m, cpkey, rkdown, &pianokeys[(m-coct)*12]);
    } else {
      draw_kbhoct(PIANO_LEFT+m*16*7, PIANO_TOP, 16, 70, m, cpkey, rkdown, NULL);
    }
  }

  // draw the ui elements on the patch page
  draw_textbox(472, DS_HEIGHT-14, 16, 180, patchname[csynth][cpatch[csynth]], patch_ui[B_PATCHNAME]);

  draw_button(310, DS_HEIGHT-14, 16, "<<", patch_ui[B_PREV]);
  sprintf(tmps, "%02d", cpatch[csynth]);
  draw_textbox(336, DS_HEIGHT-14, 16, 24, tmps, 0);
  draw_button(362, DS_HEIGHT-14, 16, ">>", patch_ui[B_NEXT]);

  draw_button(14, DS_HEIGHT-14, 16, "<<", patch_ui[B_PREVSYN]);
  sprintf(tmps, "%02d:%s",csynth, synthname[csynth]);
  draw_textbox(122, DS_HEIGHT-14, 16, 188, tmps, patch_ui[B_SYNNAME]);
  draw_button(230, DS_HEIGHT-14, 16, ">>", patch_ui[B_NEXTSYN]);
}




//
// modulator edit dialog
//
void patch_draw_modulator(void)
{
  int i,mi,j;
  char tmps[128], label[128], raws[128];
  unsigned long fmask, *fptr;
  float rf, f;

  mi=signalfifo[ csynth ][ cphover ];
  sprintf(label, "%s", modModulatorNames[mod[csynth][mi].type]);
  if (mod[csynth][mi].type==MOD_KNOB) sprintf(label, "%s", mod[csynth][mi].label);
  sprintf(tmps, "%02d %s %s", mi, modTypeNames[mod[csynth][mi].type], label);
    
  draw_textbox((DS_WIDTH/2), (DS_HEIGHT/2), 120, 226, "", 0);
  render_text(tmps, (DS_WIDTH/2)-108, (DS_HEIGHT/2)-46, 0, 0xffb05500, 0);
  render_text("esc/right click to close", (DS_WIDTH/2)+111, (DS_HEIGHT/2)+72, 2, 0xffc0c0c0, 2);

  render_text((char*)knobScaleNames[mod[csynth][mi].scale], (DS_WIDTH/2)-104, (DS_HEIGHT/2)-18, 2, 0xffc0c0c0, 0);
  draw_textbox((DS_WIDTH/2)+54, (DS_HEIGHT/2)-20, 16, 100, modeditbox, modulator_ui[B_MOD_VALUE]);  
  
  render_text("precision:", (DS_WIDTH/2)-104, (DS_HEIGHT/2)+4, 2, 0xffc0c0c0, 0);
  sprintf(tmps, "%d bits", modquantifier[ csynth ][ cpatch[csynth] ][ mi ]);
  draw_textbox((DS_WIDTH/2)+34, (DS_HEIGHT/2)+2, 16, 100, tmps, 0);
  draw_button((DS_WIDTH/2)+96, (DS_HEIGHT/2)+2, 16, "+", modulator_ui[B_MOD_ADDPREC]);
  draw_button((DS_WIDTH/2)-28, (DS_HEIGHT/2)+2, 16, "-", modulator_ui[B_MOD_DECPREC]);

  if (modModulatorTypes[mod[csynth][mi].type]==1) { // float
    i=sscanf(modeditbox, "%f", &rf);
    if (i==1) {
      rf=knob_scale2float(mod[csynth][mi].scale, rf);
      fptr=(unsigned long*)(&rf);
      fmask=0xffffffff; j=32;
      while(j>modquantifier[csynth][cpatch[csynth]][mi]) { fmask<<=1; j--; }
      *fptr&=fmask;
      f=knob_float2scale(mod[csynth][mi].scale, rf);
      sprintf(tmps, "%g", f);
      sprintf(raws, "%g", rf);
    } else {
      sprintf(tmps, "Not a float value");
      raws[0]='\0';
    }
    render_text("output:", (DS_WIDTH/2)-104, (DS_HEIGHT/2)+36, 2, 0xffc0c0c0, 0);
    render_text(tmps, (DS_WIDTH/2)+34, (DS_HEIGHT/2)+36, 2, 0xfff0f0f0, 1);

    render_text("raw:", (DS_WIDTH/2)-104, (DS_HEIGHT/2)+50, 2, 0xffc0c0c0, 0);
    render_text(raws, (DS_WIDTH/2)+34, (DS_HEIGHT/2)+50, 2, 0xfff0f0f0, 1);
  }
}

void patch_modulator_hover(int x, int y)
{
  modulator_ui[B_MOD_ADDPREC]=hovertest_box(x, y, (DS_WIDTH/2)+96, (DS_HEIGHT/2)+2, 16, 16);
  modulator_ui[B_MOD_DECPREC]=hovertest_box(x, y, (DS_WIDTH/2)-28,  (DS_HEIGHT/2)+2 , 16, 16);

  modulator_ui[B_MOD_VALUE]&=0x06;
  modulator_ui[B_MOD_VALUE]|=hovertest_box(x, y, (DS_WIDTH/2)+34,  (DS_HEIGHT/2)-20, 16, 140);
}

void patch_modulator_click(int button, int state, int x, int y)
{
  int mi,i,j;
  float f;
  unsigned long fmask, *fptr;
  
  if (state==GLUT_DOWN && !hovertest_box(x,y,(DS_WIDTH/2),(DS_HEIGHT/2),120,226 )) {
    glutIgnoreKeyRepeat(1);
    dialog_close();
    return;
  }
  
  mi=signalfifo[csynth][cphover];
  if (button==GLUT_RIGHT_BUTTON && hovertest_box(x,y,(DS_WIDTH/2),(DS_HEIGHT/2),120,226 )) {
    // read the edit box contents back to the modulator
    switch(modModulatorTypes[mod[csynth][mi].type]) {
      case 1: // float
        i=sscanf(modeditbox, "%f", &f);
        if (i==1) {
          f=knob_scale2float(mod[csynth][mi].scale, f);
          fptr=(unsigned long*)(&f);
          fmask=0xffffffff; j=32;
          while(j>modquantifier[csynth][cpatch[csynth]][mi]) { fmask<<=1; j--; }
          *fptr&=fmask;
          modvalue[csynth][cpatch[csynth]][mi]=f;
        }
        break;
      case 2: // integer
        break;
    }
    
    glutIgnoreKeyRepeat(1); dialog_close(); return; 
  }
  if (button==GLUT_LEFT_BUTTON) {
    if (state==GLUT_DOWN) {

      // remove kb focus it it was assigned
      if (modkbfocus>=0) { modulator_ui[modkbfocus]&=0x03; modkbfocus=-1; glutIgnoreKeyRepeat(1); }

      if (modulator_ui[B_MOD_ADDPREC]) {
        if (modquantifier[ csynth ][cpatch[csynth]][ mi ]<32) modquantifier[ csynth ][cpatch[csynth]][ mi ]++; return;
      }
      if (modulator_ui[B_MOD_DECPREC]) 
      {
        if (modquantifier[ csynth ][cpatch[csynth]][ mi ]>12) modquantifier[ csynth ][cpatch[csynth]][ mi ]--; return;
      }
      
      // click to edit box
      if (modulator_ui[B_MOD_VALUE]) {
        modulator_ui[B_MOD_VALUE]|=4;
        modkbfocus=B_MOD_VALUE;
        glutIgnoreKeyRepeat(0);
        return;
      }
    }
  }
}


void patch_modulator_special(int key, int x, int y)
{
  int mi;
  float f;

  /*
    left/right = change value by 1% of original value
    up/down = change value by 10% of original value
  */

  mi=signalfifo[csynth][cphover];
  switch(modModulatorTypes[mod[csynth][mi].type]) {
    case 1: //float, value is already to scale
      f=knob_float2scale(mod[csynth][mi].scale, modvalue[ csynth ][cpatch[csynth]][ mi ]);
      if (key==GLUT_KEY_RIGHT) f+=0.01 * patch_modulator_floatval;
      if (key==GLUT_KEY_LEFT) f-=0.01 * patch_modulator_floatval;
      if (key==GLUT_KEY_UP) f+=0.1 * patch_modulator_floatval;
      if (key==GLUT_KEY_DOWN) f-=0.1 * patch_modulator_floatval;
      modvalue[ csynth ][cpatch[csynth]][ mi ]=knob_scale2float(mod[csynth][mi].scale, f);
      sprintf(modeditbox, "%g", f);
    break;
    
    case 2: //integer
    break;
    
    default:
    break;
  }
}

void patch_modulator_keyboard(unsigned char key, int x, int y)
{
  int mi,i,j;
  float f;
  unsigned long fmask, *fptr;

  mi=signalfifo[csynth][cphover];
  
  // already has kb focus?
  if (modkbfocus==B_MOD_VALUE) {
    
    if (key==13) { modulator_ui[B_MOD_VALUE]&=0x03; modkbfocus=-1; glutIgnoreKeyRepeat(1); } // enter
    textbox_edit(modeditbox, key, 16);
    return;
  }
  
  if (key==13) { // enter
    switch(modModulatorTypes[mod[csynth][mi].type]) {
      case 1: // float
        i=sscanf(modeditbox, "%f", &f);
        if (i==1) {
          f=knob_scale2float(mod[csynth][mi].scale, f);
          fptr=(unsigned long*)(&f);
          fmask=0xffffffff; j=32;
          while(j>modquantifier[csynth][cpatch[csynth]][mi]) { fmask<<=1; j--; }
          *fptr&=fmask;
          modvalue[csynth][cpatch[csynth]][mi]=f;
        }
        break;
      case 2: // integer
        break;
    }
    
    glutIgnoreKeyRepeat(1); dialog_close(); return;     
  }
  if (key==27) { glutIgnoreKeyRepeat(1); dialog_close(); return; } // esc

  // some other key, focus on textbox and edit
  modulator_ui[B_MOD_VALUE]|=4;
  modkbfocus=B_MOD_VALUE; glutIgnoreKeyRepeat(0);
  textbox_edit(modeditbox, key, 16);
}    





float knob_scale2float(int scale, float value)
{
  switch(scale) {
    case SCALE_RAW: return value;
    case SCALE_FREQUENCY_HZ: return value/OUTPUTFREQ;
    case SCALE_FREQUENCY_TEMPO: return (value*bpm)/(60*OUTPUTFREQ);
    case SCALE_DURATION_TEMPO: return (60/(bpm*value))*OUTPUTFREQ; 
    case SCALE_DURATION: return value*OUTPUTFREQ;
    case SCALE_RAMP: return 1 / (value*OUTPUTFREQ);
    case SCALE_PERCENTAGE: return (value/100.0);
    case SCALE_MIDI_NOTE: return 8.1757989156 * pow(1.059463094, value) / OUTPUTFREQ;
    case SCALE_NOTE_INTERVAL: return pow(1.059463094, value);
  }
  return 0.0;
}

float knob_float2scale(int scale, float value)
{
  switch(scale) {
    case SCALE_RAW: return value;
    case SCALE_FREQUENCY_HZ: return value*OUTPUTFREQ;
    case SCALE_FREQUENCY_TEMPO: return (value*60*OUTPUTFREQ)/bpm;
    case SCALE_DURATION_TEMPO: return (OUTPUTFREQ*60)/(value*bpm);
    case SCALE_DURATION: return value/OUTPUTFREQ;
    case SCALE_RAMP: return 1 / (value*OUTPUTFREQ);
    case SCALE_PERCENTAGE: return (value*100.0);
    case SCALE_MIDI_NOTE: return 17.31234049667*log(0.12231220586*value*OUTPUTFREQ);
    case SCALE_NOTE_INTERVAL: return 17.31234049667*log(value);
  }
  return 0.0;
}
