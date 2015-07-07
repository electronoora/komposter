/*
 * Komposter (c) 2010-2011 Jani Halme (Firehawk/TDA)
 *
 * This code is licensed under the GNU General Public
 * License version 2. See file 'doc/LICENSE'.
 *
 * Main program and thread startup
 *
 * $Rev$
 * $Date$
 */

#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <strings.h>
#include <sys/time.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>

#ifdef __APPLE__
#include <CoreFoundation/CFURL.h>
#include <CoreFoundation/CFBundle.h>
#endif

#include "arch.h"
#include "about.h"
#include "audio.h"
#include "buffermm.h"
#include "constants.h"
#include "console.h"
#include "dotfile.h"
#include "dialog.h"
#include "font.h"
#include "modules.h"
#include "pattern.h"
#include "patch.h"
#include "widgets.h"
#include "sequencer.h"
#include "synthesizer.h"


#define MAIN_ABOUT 0
#define MAIN_PAGE1 1
#define MAIN_PAGE2 2
#define MAIN_PAGE3 3
#define MAIN_PAGE4 4

#define MAIN_VU	5

int cpage=1;
int main_ui[6];

float lastrf=0.0f;

// posix threads for audio playback and rendering
pthread_t audiothread;
pthread_t renderthread;

// path to resources
char respath[512];

// from audio.c
extern short *render_buffer;
extern int render_state;
extern long render_pos;
extern long render_bufferlen;
long audio_render(void);
extern float audio_peak;
extern float audio_latest_peak;

// from modules.c
//extern double osc_offset[7];
//extern double coeftable[12];
//extern float supersaw_detune[128][7];
//extern float supersaw_mix[128][7];



void mouse_hoverfunc(int x, int y)
{
  int m;

  if (is_dialog()) { dialog_hover(x,y); return; }

  // test global ui elements
  for(m=0;m<5;m++) main_ui[m]=0;
  main_ui[MAIN_PAGE1]=hovertest_box(x,y,DS_WIDTH-159, DS_HEIGHT-14, 16, 16);
  main_ui[MAIN_PAGE2]=hovertest_box(x,y,DS_WIDTH-137, DS_HEIGHT-14, 16, 16);
  main_ui[MAIN_PAGE3]=hovertest_box(x,y,DS_WIDTH-115, DS_HEIGHT-14, 16, 16);
  main_ui[MAIN_PAGE4]=hovertest_box(x,y,DS_WIDTH-93, DS_HEIGHT-14, 16, 16);
  main_ui[MAIN_ABOUT]=hovertest_box(x,y,DS_WIDTH-42,DS_HEIGHT-14, 16, 73);

  main_ui[MAIN_VU]=hovertest_box(x,y,728, DS_HEIGHT-14, 16, 100);
  
  // call the hover function of the currently active page
  switch(cpage) {
    case MAIN_PAGE1: synth_mouse_hover(x,y); break;
    case MAIN_PAGE2: patch_mouse_hover(x,y); break;
    case MAIN_PAGE3: pattern_mouse_hover(x,y); break;
    case MAIN_PAGE4: sequencer_mouse_hover(x,y); break;
    default: break;
  }
}
    
void mouse_dragfunc(int x, int y)
{
  if (is_dialog()) {
    if (is_dialogdrag()) {
      dialog_drag(x, y);
    }
    return;
  }

  switch(cpage) {
    case MAIN_PAGE1: synth_mouse_drag(x,y); break;
    case MAIN_PAGE2: patch_mouse_drag(x,y); break;
    case MAIN_PAGE3: pattern_mouse_drag(x,y); break;
    case MAIN_PAGE4: sequencer_mouse_drag(x,y); break;
    default: break;
  }
}

void mouse_clickfunc(int button, int state, int x, int y)
{
  if (is_dialog()) { dialog_click(button,state,x,y); return; }

  // test global ui elements
  if (button==GLUT_LEFT_BUTTON) {
    if (state==GLUT_DOWN) {
      if (main_ui[MAIN_PAGE1]) { console_post("Synthesizers"); cpage=1; return; }
      if (main_ui[MAIN_PAGE2]) { console_post("Patches");      cpage=2; return; }      
      if (main_ui[MAIN_PAGE3]) { console_post("Patterns");     cpage=3; return; }
      if (main_ui[MAIN_PAGE4]) { console_post("Sequencer");    cpage=4; return; }
      if (main_ui[MAIN_ABOUT]) { dialog_open(&about_draw, &about_hover, &about_click); return; }
      if (main_ui[MAIN_VU]) { audio_peak=0.0f; console_post("VU meter peak reset"); return; }
    }
  }

  switch (cpage)
  {
    case MAIN_PAGE1: synth_mouse_click(button, state, x, y); break;
    case MAIN_PAGE2: patch_mouse_click(button, state, x, y); break;
    case MAIN_PAGE3: pattern_mouse_click(button, state, x, y); break;
    case MAIN_PAGE4: sequencer_mouse_click(button, state, x, y); break;
    default: break;
  }
}

void keyboardfunc(unsigned char key, int x, int y)
{
  if (is_dialog()) { dialog_keyboard(key, x, y); return; }
  switch (cpage)
  {
    case MAIN_PAGE1: synth_keyboard(key, x, y); break;
    case MAIN_PAGE2: patch_keyboard(key, x, y); break;
    case MAIN_PAGE3: pattern_keyboard(key, x, y); break;
    case MAIN_PAGE4: sequencer_keyboard(key, x, y); break;
    default: break;
  }
}

void keyboardupfunc(unsigned char key, int x, int y)
{
  if (is_dialog()) return;
  switch (cpage)
  {
    case MAIN_PAGE2: patch_keyboardup(key, x, y); break;
    case MAIN_PAGE3: pattern_keyboardup(key, x, y); break;
    default: break;
  }
}

void specialkeyfunc(int key, int x, int y)
{
  if (is_dialog()) { dialog_special(key, x, y); return; }

  // page handlers for special keys
  switch (cpage)
  {
    case MAIN_PAGE1: synth_specialkey(key, x, y); break;
    case MAIN_PAGE2: patch_specialkey(key, x, y); break;
    case MAIN_PAGE3: pattern_specialkey(key, x, y); break;
    case MAIN_PAGE4: break;
  }

  // global keys
  switch(key) {
    case GLUT_KEY_F1:
      if (cpage!=1) { console_post("Synthesizers"); cpage=1;}
      break;
    case GLUT_KEY_F2:
      if (cpage!=2) { console_post("Patches"); cpage=2; }      
      break;
    case GLUT_KEY_F3:
      if (cpage!=3) { console_post("Patterns"); cpage=3; }
      break;
    case GLUT_KEY_F4:
      if (cpage!=4) { console_post("Sequencer"); cpage=4; }
      break;
    default: break;
  } 

}


void update(int value)
{
  console_advanceframe();
  glutTimerFunc(20, update, value+1); // frame number in callback parameter
  glutPostRedisplay();
}



void *audio_playback(void *param)
{
  int rc;

  while(1) {
    audio_update(0);
    rc=usleep(1000); // 1ms sleep
  }
  return NULL;
}

void *audio_renderer(void *param)
{
  int rc;

  while(1) {
    if (render_state==RENDER_IN_PROGRESS || render_state==RENDER_LIVE) {
      audio_render();
    } else {
      rc=usleep(10000);
    }
  }
  return NULL;
}


void display(void)
{
  char tmps[128];
  float rf;

  // setup projection and modelview matrices
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(0, DS_WIDTH, 0, DS_HEIGHT, -1, 1);
  glScalef(1, -1, 1);
  glTranslatef(0, -DS_HEIGHT, 0);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  // clear the back buffer
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  // enable stuff
  glEnable(GL_LINE_SMOOTH);
  glEnable(GL_BLEND);  
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);

  // draw the current page
  switch (cpage)
  {
    case MAIN_PAGE1: synth_draw(); break;
    case MAIN_PAGE2: patch_draw(); break;
    case MAIN_PAGE3: pattern_draw(); break;
    case MAIN_PAGE4: sequencer_draw(); break;
    default: break;
  }

  // global ui elements
  console_print(6, DS_HEIGHT-27);
  main_ui[cpage]|=2;
  draw_button(DS_WIDTH-159, DS_HEIGHT-14, 16, "1", main_ui[MAIN_PAGE1]);
  draw_button(DS_WIDTH-137, DS_HEIGHT-14, 16, "2", main_ui[MAIN_PAGE2]);
  draw_button(DS_WIDTH-115, DS_HEIGHT-14, 16, "3", main_ui[MAIN_PAGE3]);
  draw_button(DS_WIDTH-93, DS_HEIGHT-14, 16, "4", main_ui[MAIN_PAGE4]);
  draw_textbox(DS_WIDTH-42, DS_HEIGHT-14, 16, 74, "komposter", main_ui[MAIN_ABOUT]);
  main_ui[cpage]&=1;

  // draw vu peak meter
  rf=fmin(1.0f, audio_latest_peak);
  rf=(0.2*lastrf) + (0.8*rf); // damping
  lastrf=rf;
  draw_textbox(728, DS_HEIGHT-14, 16, 100, "", main_ui[MAIN_VU]);
  glColor4f(0.68f, 0.33f, 0.0f, 0.94f);
  glBegin(GL_QUADS);
  glVertex2f(678, DS_HEIGHT-22);
  glVertex2f(678 + rf*100, DS_HEIGHT-22);
  glVertex2f(678 + rf*100, DS_HEIGHT-6);
  glVertex2f(678, DS_HEIGHT-6);
  glEnd();
  sprintf(tmps, "%1.2f", audio_peak);
  render_text(tmps, 728, DS_HEIGHT-11, 2, (audio_peak > 1.0f) ? 0xffff8080 : 0xffffffff, 1);

  // if a dialog is active, dim the screen and draw it
  if (is_dialog()) dialog_draw();

  // switch the back buffer to front
  glutSwapBuffers();
}


void cleanup(void)
{
  printf("Closing up..\n");
  pthread_kill(audiothread, SIGTERM);
  pthread_kill(renderthread, SIGTERM);
  pthread_exit(NULL);
  audio_release();
}


int is_dir(const char *path)
{
	struct stat st;

	if(stat(path, &st) == 0)
	{
		if((st.st_mode & S_IFDIR) != 0)
		{
			return 1;
		}
	}

	return 0;
}

int main(int argc, char **argv)
{
  int err;
#ifdef __APPLE__
  char *cfrespath;
  CFBundleRef mainBundle;
  CFURLRef res;
  CFStringRef respathref;
#endif

  // load config file from user's homedir
  dotfile_load();

  // calculate tables for supersaw
  calc_supersaw_tables();

  // init data on all pages to defaults
  synth_init();
  patch_init();
  pattern_init();
  sequencer_init();

  // init glut
  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
  glutInitWindowSize(DS_WIDTH,DS_HEIGHT);
  glutCreateWindow("komposter");
  glutIgnoreKeyRepeat(1);
  
  

#ifdef __APPLE__
  // get application bundle base path
  mainBundle=CFBundleGetMainBundle();
  res=CFBundleCopyBundleURL(mainBundle);
  respathref=CFURLCopyFileSystemPath(res, 0);
  cfrespath=(char*)CFStringGetCStringPtr(respathref, kCFStringEncodingISOLatin1);
  if (!cfrespath) {
    cfrespath=malloc(512);
    CFStringGetCString(respathref, cfrespath, 511, kCFStringEncodingISOLatin1);
    strncpy(respath, cfrespath, 511);
    free(cfrespath);
  } else {
    strncpy(respath, cfrespath, 511);  
  }
  strncat(respath, "/Contents/Resources/", 511); // append the resource dir
#else
#ifdef RESOURCEPATH
	if(is_dir(RESOURCEPATH))
	{
		strncpy(respath, RESOURCEPATH, 511);
	}
	else
	{
		fprintf(stderr, "'%s' not found, trying relative path\n", RESOURCEPATH);
		strncpy(respath, "resources/", 511);
	}
#else
  strncpy(respath, "resources/", 512);
#endif
#endif
  printf("Resource path is %s\n", respath);

  // init freetype
  err=font_init();
  if (!err) {
    printf("Error initializing Freetype!\n");
    return 0;
  }

  // set glut callbacks
  glutKeyboardFunc(keyboardfunc);
  glutKeyboardUpFunc(keyboardupfunc);
  glutMouseFunc(mouse_clickfunc);
  glutMotionFunc(mouse_dragfunc);
  glutSpecialFunc(specialkeyfunc);
  glutPassiveMotionFunc(mouse_hoverfunc);

  // init memory manager
  kmm_init();

  // set up screen and fire up the update timer
  cpage=MAIN_PAGE4;
  glutDisplayFunc(display);
  glutTimerFunc(20, update, 1);
  dialog_open(&about_draw, &about_hover, &about_click);

  // start audio and opengl mainloop
  atexit(cleanup);
  if (!audio_initialize()) {
    printf("Failed to initialize audio playback - sound is disabled.\n");
  } else {
    err = pthread_create(&audiothread, NULL, audio_playback, (void *)NULL);
  }
  err = pthread_create(&renderthread, NULL, audio_renderer, (void *)NULL);
  glutMainLoop();
  return 0;
}
