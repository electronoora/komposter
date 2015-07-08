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

#include "dialog.h"
#include "filedialog.h"

void filedialog_open(filedialog *fd, char *ext, char *path)
{
  int i, r;
  struct stat sb;
  char *home;

  // set glob wildcard
  strncpy((char*)&fd->fmask, "*.", 255);
  strncat((char*)&fd->fmask, ext, 255);

  // path to use initially
  if (path) {
    r=stat(path, &sb);
    if (!r) {
      strcpy((char*)(&fd->cpath), path);
    } else {
      home=getenv("HOME");
      strcpy((char*)&fd->cpath, home);
    }
  } else {
    strcpy((char*)(&fd->cpath), "");
  }

//  printf("opened filedialog mask=%s path=%s\n", fd->fmask, fd->cpath);

  fd->owconfirm=0;
  fd->kbfocus=0;
  fd->listhover=-1;
  fd->exitstate=0;
  fd->sliderdrag=0;
  fd->sliderstep=12;
  fd->sliderpos=0;
  for(i=0;i<4;i++) fd->hover[i]=0;
    
  // scan files in the path
  filedialog_scanpath(fd);
}


// ghh, all the nice glob options in linux are not POSIX, so can't
// use them..
void filedialog_scanpath(filedialog *fd)
{
  char globp[512];

  // release memory from previous glob
  if (fd->g.gl_pathc) globfree(&fd->g);

  // glob like HELL!  \,,/
  strncpy((char*)&globp, fd->cpath, 512);
  strncat((char*)&globp, "/..", 512);
//  glob(globp, GLOB_MARK | GLOB_TILDE | GLOB_ONLYDIR | GLOB_PERIOD, NULL, &fd->g);
//printf("globbing from %s\n", globp);
  glob(globp, GLOB_MARK | GLOB_TILDE, NULL, &fd->g);

  strncpy((char*)&globp, fd->cpath, 512);
  strncat((char*)&globp, "/*", 512);
//  glob(globp, GLOB_MARK | GLOB_TILDE | GLOB_ONLYDIR | GLOB_APPEND, NULL, &fd->g);
//printf("globbing from %s\n", globp);
  glob(globp, GLOB_MARK | GLOB_TILDE | GLOB_APPEND, NULL, &fd->g);

//  strncpy((char*)&globp, fd->cpath, 512);
//  strncat((char*)&globp, "/", 512);
//  strncat((char*)&globp, fd->fmask, 512);
//  glob(globp,     GLOB_MARK | GLOB_TILDE | GLOB_APPEND,  NULL, &fd->g);
} 



void filedialog_draw(filedialog *fd)
{
  int i;
  char tmps[255], ttmps[255];
  unsigned long fcol;

  draw_textbox((DS_WIDTH/2), (DS_HEIGHT/2), 244, 420, "", 0);
  render_text(fd->title, (DS_WIDTH/2)-196, (DS_HEIGHT/2)-102, 0, 0xffb05500, 0);
  render_text("esc/right click to close", (DS_WIDTH/2)+204, (DS_HEIGHT/2)-100, 2, 0xff707070, 2);

  draw_textbox((DS_WIDTH/2)-10, (DS_HEIGHT/2)-8, 160, 370, "", 0);
  draw_vslider((DS_WIDTH/2)+184.5, (DS_HEIGHT/2)-89.5, 12, 160,
    fd->sliderpos, 12, fd->g.gl_pathc,
    fd->hover[FDUI_VSLIDER]);

  i=fd->sliderpos;
  while ( (i<fd->g.gl_pathc) && (i<(fd->sliderpos+12)) ) {
    if (fd->listhover>=0 && fd->listhover == (i-fd->sliderpos)) {
      // highlight
      glColor4f(0.3f, 0.3f, 0.3f, 0.5f);
      glBegin(GL_QUADS);
      glVertex2f( ((DS_WIDTH/2)-195),  ((DS_HEIGHT/2)-88) + (i-fd->sliderpos)*13);
      glVertex2f( ((DS_WIDTH/2)-195)+370,  ((DS_HEIGHT/2)-88) + (i-fd->sliderpos)*13);
      glVertex2f( ((DS_WIDTH/2)-195)+370,  ((DS_HEIGHT/2)-88) + (i-fd->sliderpos)*13+13);
      glVertex2f( ((DS_WIDTH/2)-195),  ((DS_HEIGHT/2)-88) + (i-fd->sliderpos)*13+13);      
      glEnd();
    }
    if (strlen(strrchr(fd->g.gl_pathv[i], '/'))==1)
    { // directory
      strncpy(ttmps, fd->g.gl_pathv[i], strlen(fd->g.gl_pathv[i])-1);
      ttmps[strlen(fd->g.gl_pathv[i])-1]='\0';
      strncpy(tmps, rindex(ttmps, '/')+1, 255);
      render_text(tmps,
        (DS_WIDTH/2)-192, 
        (DS_HEIGHT/2)-78+(i-fd->sliderpos)*13, 2, 0xcfa0a0c0, 0); 
      render_text("<dir>",
        (DS_WIDTH/2)+162, 
        (DS_HEIGHT/2)-78+(i-fd->sliderpos)*13, 2, 0xcfa0a0c0, 2); 
    } else { // file
      fcol=0xffc0c0c0;
      if (!strstr(fd->g.gl_pathv[i], fd->fmask+1)) fcol=0xff909090;
      strncpy(tmps, strrchr(fd->g.gl_pathv[i], '/')+1, 255);
      render_text(tmps,
        (DS_WIDTH/2)-192, 
        (DS_HEIGHT/2)-78+(i-fd->sliderpos)*13, 2, fcol, 0); 
    }
    i++;
  }

  render_text("Filename:", (DS_WIDTH/2)-198, (DS_HEIGHT/2)+103, 2, 0xffc0c0c0, 0);
  draw_textbox((DS_WIDTH/2)+20, (DS_HEIGHT/2)+100, 16, 300, fd->fname, fd->hover[FDUI_FILENAME]+fd->kbfocus);
  draw_button((DS_WIDTH/2)+192, (DS_HEIGHT/2)+100, 16, "OK",  fd->hover[FDUI_OK]);
}



void filedialog_hover(filedialog *fd, int x, int y)
{
  fd->hover[FDUI_FILENAME]=hovertest_box(x, y, (DS_WIDTH/2)+20, (DS_HEIGHT/2)+100, 16, 300);
  fd->hover[FDUI_OK]=hovertest_box(x, y, (DS_WIDTH/2)+192, (DS_HEIGHT/2)+100, 16, 16);
  fd->hover[FDUI_VSLIDER]=hovertest_vslider(x, y, 
    (DS_WIDTH/2)+184.5, (DS_HEIGHT/2)-89.5, 12, 160,
    fd->sliderpos, 12, fd->g.gl_pathc);
    
  fd->listhover=-1;
  if (hovertest_box(x, y, (DS_WIDTH/2)-10, (DS_HEIGHT/2)-8, 160, 370)) {
    // hovering on listbox
    fd->listhover= (y-((DS_HEIGHT/2)-88)) / 13;
  }
}



void filedialog_click(filedialog *fd, int button, int state, int x, int y)
{
  int r;
  struct stat sb;
  char tmps[255], ttmps[255], *tptr;;

  if (state==GLUT_DOWN && !hovertest_box(x, y, (DS_WIDTH/2), (DS_HEIGHT/2), 244, 420)) { dialog_close(); return; }

  if (state==GLUT_UP) { fd->sliderdrag=0; return; }

  fd->kbfocus=0;
  glutIgnoreKeyRepeat(1);

  if (button==GLUT_RIGHT_BUTTON && state==GLUT_DOWN) { fd->exitstate=FDEXIT_CANCEL; return; }

  if (fd->hover[FDUI_FILENAME]) { fd->kbfocus=4; glutIgnoreKeyRepeat(0); return; }
  if (fd->hover[FDUI_OK]) {
    strncpy(fd->fullpath, fd->cpath, 512);
    strncat(fd->fullpath, "/", 512);
    strncat(fd->fullpath, fd->fname, 512);  
    if (!strstr(fd->fullpath, fd->fmask+1)) {
      // append the extension
      strncat(fd->fullpath, fd->fmask+1, 512);
    }    
    // printf("exiting dialog with fn %s\n", fd->fullpath);    
    fd->exitstate=FDEXIT_OK; 
    return;
  }
  if (fd->hover[FDUI_VSLIDER]) {
    fd->sliderdrag=1;
    fd->slider_yofs=y;
    fd->slider_dragstart=fd->sliderpos;
  }

  // scrollwheel on listbox - need to use a patched glut for this
  /*
  if (button==GLUT_WHEEL_UP && fd->listhover>=0) if (fd->sliderpos>0) fd->sliderpos--;
  if (button==GLUT_WHEEL_DOWN && fd->listhover>=0) if (fd->sliderpos<(fd->g.gl_pathv-12)) fd->sliderpos++;
  */

  // click on listbox
  if (button==GLUT_LEFT_BUTTON && state==GLUT_DOWN && fd->listhover>=0) {
    r=stat(fd->g.gl_pathv[fd->sliderpos+fd->listhover], &sb);
    switch (sb.st_mode&S_IFMT) {
      case S_IFDIR:
        strncpy(ttmps, fd->g.gl_pathv[fd->sliderpos+fd->listhover], strlen(fd->g.gl_pathv[fd->sliderpos+fd->listhover])-1);
        ttmps[strlen(fd->g.gl_pathv[fd->sliderpos+fd->listhover])-1]='\0';
        strncpy(tmps, rindex(ttmps, '/')+1, 255);
        if (!strcmp(tmps, "..")) {
          tptr=rindex(ttmps, '/');
          if (tptr) {
            *tptr='\0';
            tptr=rindex(ttmps, '/');
            if (tptr) {
              *tptr='\0';
            } else {
              tptr=ttmps;
            }
          } else {
            tptr=ttmps;
          }
        }          
        strncpy(fd->cpath, ttmps, 255);
        filedialog_scanpath(fd);
        fd->sliderpos=0;
        fd->listhover=0;
        break;
      case S_IFREG:
        strncpy(fd->fname, strrchr(fd->g.gl_pathv[fd->sliderpos+fd->listhover], '/')+1, 255);
        break;
      default:
        // not a dir or file so disregard
        break;
    }
  }
}



void filedialog_keyboard(filedialog *fd, int key)
{
  if (fd->kbfocus) {
    if (key==13) { fd->kbfocus=0; glutIgnoreKeyRepeat(1); return; }
    textbox_edit(fd->fname, key, 45);
    return;
  }
  if (key==13) {
    strncpy(fd->fullpath, fd->cpath, 512);
    strncat(fd->fullpath, "/", 512);
    strncat(fd->fullpath, fd->fname, 512);
    if (!strstr(fd->fullpath, fd->fmask+1)) {
      // append the extension
      strncat(fd->fullpath, fd->fmask+1, 512);
    }
    fd->exitstate=FDEXIT_OK; 
    return;
  }
  if (key==27) { fd->exitstate=FDEXIT_CANCEL; return; }
  
  // some other key, focus on textbox and edit
  fd->kbfocus=4; glutIgnoreKeyRepeat(0);
  textbox_edit(fd->fname, key, 45);
}


void filedialog_drag(filedialog *fd, int x, int y)
{
  float sbh, cos, cip, slh, f;

  // dragging the scrollbar
  if (fd->sliderdrag) {
    sbh=160; // total height of scrollbar
    cos=12; // lines on box
    cip=fd->g.gl_pathc; // files total
    slh=sbh*(cos/cip); // slider height
    f= ( (cip-cos) * (y - fd->slider_yofs) )  /  (sbh-slh) ;
    fd->sliderpos=fd->slider_dragstart+f;
    if (fd->sliderpos>(cip-cos)) fd->sliderpos=(cip-cos);
    if (fd->sliderpos<0) fd->sliderpos=0;
    return;
  }                                          
}
