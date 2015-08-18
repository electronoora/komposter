/*
 * Komposter (c) 2010-2011 Jani Halme (Firehawk/TDA)
 *
 * This code is licensed under the GNU General Public
 * License version 2. See file 'doc/LICENSE'.
 *
 * Utility functions for GUI widgets
 *
 * $Rev$
 * $Date$
 */

#include "widgets.h"


//void draw_module(float x, float y, int type)
void draw_module(synthmodule *s)
{
  int i;
  float x,y;int type;
  float size;

  x=s->x;y=s->y;type=s->type;
  
  // background
  if (s->active) {
    glColor4f(0.1f, 0.1f, 0.1f, 0.94f);
  } else {
    glColor4f(0.05f, 0.05f, 0.05f, 0.92f);
  }
  glBegin(GL_QUADS);
  glVertex2f(x-MODULE_HALF, y-MODULE_HALF);  glVertex2f(x+MODULE_HALF, y-MODULE_HALF);
  glVertex2f(x+MODULE_HALF, y+MODULE_HALF);  glVertex2f(x-MODULE_HALF, y+MODULE_HALF);
  glEnd();

  // surrounding frame
  size=MODULE_SIZE+1;
  glLineWidth(1.0f);
  glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
  glBegin(GL_LINE_LOOP);
  glVertex2f(x-(size/2), y-(size/2));  glVertex2f(x+(size/2), y-(size/2));
  glVertex2f(x+(size/2), y+(size/2));  glVertex2f(x-(size/2), y+(size/2));
  glEnd();

  // label
  if (s->type!=MOD_KNOB && s->type!=MOD_ATTENUATOR && s->type!=MOD_OUTPUT && s->type!=MOD_ACCENT) {
    render_text((char*)modTypeNames[s->type], x+1, round(y+(MODULE_SIZE/5))+1, 2, 0xff000000, 1);
    render_text((char*)modTypeNames[s->type], x, round(y+(MODULE_SIZE/5)), 2, 0xffffffff, 1);
  } else {
    // label
    switch (s->type) {
      case MOD_OUTPUT:
        draw_knob(x, y+(MODULE_SIZE/15), 0.3f, 0.3f, 0.5f, 0.4f);
        break;
      case MOD_ACCENT:
        draw_knob(x, y+(MODULE_SIZE/15), 0.3f, 0.5f, 0.3f, 0.3f);
        break;      
      default:
        draw_knob(x, y+(MODULE_SIZE/15), 0.3f, 0.4f, 0.5f, 0.5f);
        break;
    }
    glColor4f(0.8, 0.8, 0.8, 1.0);
    if (s->label && strlen(s->label)>0) {
      render_text(s->label, x+1, round(y+(MODULE_SIZE/3)+5), 2, 0xff000000, 1);
      render_text(s->label, x, round(y+(MODULE_SIZE/3)+4), 2, 0xffffffff, 1);
    } else {
      render_text((char*)modTypeNames[s->type], x+1, round(y+(MODULE_SIZE/3)+5), 2, 0xff000000, 1);
      render_text((char*)modTypeNames[s->type], x, round(y+(MODULE_SIZE/3)+4), 2, 0xffffffff, 1);
    }
  }

  // draw input nodes
  for(i=0;i<modInputCount[type];i++) {
    draw_signal_node(
      x + node_xoffset[modInputCount[type]][i],
      y + node_yoffset[modInputCount[type]][i],
      (s->inpactive==i) ? 2 : 0,
      (char*)modInputNames[type][i], node_labelpos[modInputCount[type]][i]);
  }

  // draw output node
  if (type!=MOD_OUTPUT) {
    if (s->outactive) {
      draw_signal_node(x+MODULE_HALF+0.5, y+OUTPUT_OFFSET, 3, "Out", 1);
    } else {
      draw_signal_node(x+MODULE_HALF+0.5, y+OUTPUT_OFFSET, 1, "Out", 1);    
    }
  }
}



// type 0=input, 1=output, 2=active input, 3=active output
// label align 0=below, 1=to left, 2=above, 3=to right
void draw_signal_node(float x, float y, int type, char* label, int align)
{
  float f,r;

  // output nodes are red and offset 
  if (type&1) {
    glColor3f(1, 0.2, 0.2);
  } else {
    glColor3d(1, 1, 1);
  }

  // highlight the node if active
  r=NODE_RADIUS;
  glLineWidth(1.0f);
  if (type&2) { r*=1.5f; glLineWidth(2.0f); }

  // draw node
  glBegin(GL_LINE_LOOP);
  for(f=0;f<1.0f;f+=(1.0/8)) {
    glVertex3f(x + cos(f*2*M_PI)*(r*0.75), y + sin(f*2*M_PI)*(r*0.75), 0.0f);
  }    
  glEnd();

  // draw label
  glColor4f(0.6, 0.6, 0.6, 1.0);
  switch(align)
  {
    case 1: // to left
    render_text(label, x-5.5, y+3, 2, 0xffa0a0a0, 2);
    break;

    case 2: // above
    render_text(label, x, y-5.5, 2, 0xffa0a0a0, 1);
    break;

    case 3: // to right
    render_text(label, x+5.5, y+3, 2, 0xffa0a0a0, 0);
    break;
    
    default: // below
    render_text(label, x, y+10.5, 2, 0xffa0a0a0, 1);
    break;
  }

}


// draw a round knob for setting [0.0, 1.0]
//void draw_knob(knob *k, float x, float y)
void draw_knob(float x, float y, float setting, float r, float g, float b)
{
  float f,c;

  // knob's beveled edge
  glBegin(GL_QUAD_STRIP);
  for(f=0;f<1.0f;f+=(1.0/20)) {
    c=0.2+(1.0f+cos(0.6+f*2*M_PI))/(2.0);
    glColor3f(c*r, c*g, c*b);
    glVertex3f(x + cos(f*2*M_PI)*KNOB_RADIUS, y + sin(f*2*M_PI)*KNOB_RADIUS, 0.0f);
    glVertex3f(x + cos(f*2*M_PI)*(KNOB_RADIUS*0.75), y + sin(f*2*M_PI)*(KNOB_RADIUS*0.75), 0.0f);
  }  
  glVertex3f(x + KNOB_RADIUS, y, 0.0f);
  glVertex3f(x + (KNOB_RADIUS*0.75), y, 0.0f);
  glEnd();
  
  // flat center on the knob
  glBegin(GL_POLYGON);
  glColor3f(r, g, b);
  for(f=0;f<1.0f;f+=(1.0/20)) {
    glVertex3f(x + cos(f*2*M_PI)*(KNOB_RADIUS*0.75), y + sin(f*2*M_PI)*(KNOB_RADIUS*0.75), 0.0f);
  }    
  glEnd();

  // position indicator on knob
  glLineWidth(3.0f);
  glBegin(GL_LINES);
  glColor3f(0.0f, 0, 0);
  glVertex2f(x,y); glVertex2f(x + cos(setting*2*M_PI)*KNOB_RADIUS, y + sin(setting*2*M_PI)*KNOB_RADIUS);
  glEnd();
  glLineWidth(1.0f);  
  glBegin(GL_LINES);
  glColor3f(1.0f, 0, 0);
  glVertex2f(x,y); glVertex2f(x + cos(setting*2*M_PI)*KNOB_RADIUS, y + sin(setting*2*M_PI)*KNOB_RADIUS);
  glEnd();
}



// draw bitmapped text
// align 0=left, 1=center, 2=right
void draw_text(char *dbtext, float x, float y, char align)
{
  char *c;
  float x1,xd;

  // text width
  xd=0; 
  for (c=dbtext; *c != '\0'; c++) {
    xd += glutBitmapWidth(MODULE_FONT,*c); 
  }

  // align
  x1=x;
  switch (align) {
    case 1: x1-=(xd/2); break;
    case 2: x1-=xd; break;
  }

  // draw
  for (c=dbtext; *c != '\0'; c++) {
    glRasterPos3f(x1, y, 0.0f);
    glutBitmapCharacter(MODULE_FONT, *c);
    x1 += glutBitmapWidth(MODULE_FONT,*c);
  }
}



// draw a patch "cable" between two points
void draw_patch(float x1, float y1, float x2, float y2)
{
  pt p;
  bzr b;
  float s;

  // endpoints
  b.x0=x1; b.y0=y1;
  b.x1=x2; b.y1=y2;

  // control points
  b.cx1=x2;
  b.cy1=y1;
  b.cx0=x2;
  b.cy0=y1;

  // draw a bezier curve
  glColor4f(1.0, 1.0, 1.0, 0.8f);
  glLineWidth(2.0f);
  glBegin(GL_LINE_STRIP);
  for(s=0;s<1.0;s+=0.05) {
    bezier(&p, &b, s);    
    glVertex2f(p.x, p.y);
  }
  glVertex2f(x2,y2);
  glEnd();
}


// draw a patch "cable" between two points
void draw_patch_control(float x1, float y1, float x2, float y2, float cx1, float cy1, float cx2, float cy2)
{
  pt p;
  bzr b;
  float s;

  // endpoints
  b.x0=x1; b.y0=y1;
  b.x1=x2; b.y1=y2;

  // control points
  b.cx1=cx2;
  b.cy1=cy2;
  b.cx0=cx1;
  b.cy0=cy1;

  // draw a bezier curve
  glColor4f(1.0, 1.0, 1.0, 0.8f);
  glLineWidth(2.0f);
  glBegin(GL_LINE_STRIP);
  for(s=0;s<1.0;s+=0.05) {
    bezier(&p, &b, s);    
    glVertex2f(p.x, p.y);
  }
  glVertex2f(x2,y2);
  glEnd();
}


void draw_button(float x, float y, float size, char *label, int type)
{
  // background
  glColor4f(0.05f, 0.05f, 0.05f, 0.92f);  
  if (type&1) { glColor4f(0.15f, 0.15f, 0.15f, 0.94f); } // hover
  if (type&2) { glColor4f(0.68f, 0.33f, 0.0f, 0.94f); } // enabled
  if (type&8) { glColor4f(0.5f, 0.0f, 0.0f, 0.94f); } // confirm
  
  glBegin(GL_QUADS);
  glVertex2f(x-(size/2), y-(size/2));  glVertex2f(x+(size/2), y-(size/2));
  glVertex2f(x+(size/2), y+(size/2));  glVertex2f(x-(size/2), y+(size/2));
  glEnd();

  // surrounding frame
  size+=1; // to avoid subpixel positioning
  glLineWidth(1.0f);
  glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
  glBegin(GL_LINE_LOOP);
  glVertex2f(x-(size/2), y-(size/2));  glVertex2f(x+(size/2), y-(size/2));
  glVertex2f(x+(size/2), y+(size/2));  glVertex2f(x-(size/2), y+(size/2));
  glEnd();

  // label
  render_text(label, x, y+3, 2, 0xffffffff, 1);
}


void draw_textbox(float x, float y, float height, float length, char *label, int type)
{
  // background
    glColor4f(0.05f, 0.05f, 0.05f, 0.92f);
  if (type&1) glColor4f(0.15f, 0.15f, 0.15f, 0.94f);
  if (type&2) glColor4f(0.68f, 0.33f, 0.0f, 0.94f);  // enabled
  if (type&4) glColor4ub(0x29, 0x42, 0xAB, 0xf0); // keyboard focus
  if (type&8) { glColor4f(0.5f, 0.0f, 0.0f, 0.94f); } // confirm
  
  glBegin(GL_QUADS);
  glVertex2f(x-(length/2), y-(height/2));  glVertex2f(x+(length/2), y-(height/2));
  glVertex2f(x+(length/2), y+(height/2));  glVertex2f(x-(length/2), y+(height/2));
  glEnd();

  // surrounding frame
  height++; length++; // avoid subpixel positioning
  glLineWidth(1.0f);
  glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
  glBegin(GL_LINE_LOOP);
  glVertex2f(x-(length/2), y-(height/2));  glVertex2f(x+(length/2), y-(height/2));
  glVertex2f(x+(length/2), y+(height/2));  glVertex2f(x-(length/2), y+(height/2));
  glEnd();

  // label
  render_text(label, x, y+3, 2, 0xffffffff, 1);
}



void draw_dimmer(void)
{
  glColor4f(0.0f, 0.0f, 0.0f, 0.85f);
  glBegin(GL_QUADS);
  glVertex2f(0.0f, 0.0f); glVertex2f(0.0f, DS_HEIGHT);;
  glVertex2f(DS_WIDTH, DS_HEIGHT); glVertex2f(DS_WIDTH, 0);
  glEnd();
}


// draw an octave of vertical keyboard on the side of the screen
void draw_kboct(float y, float kw, float kh, int oct, int hlkey, int hlkeydown)
{
  int i, j, ckey;
  char tmps[255];
  char wkj[]={2,2,1,2,2,2,1};
  char bkj[]={2,3,2,2};

  // draw an octave of keyboard
  ckey=oct*12;
  for (j=0;j<7;j++) { // white keys
    glBegin(GL_QUADS);
    if (ckey==hlkey) { glColor4f(1.0, 1.0, 1.0, 0.82);
    } else { glColor4f(1.0, 1.0, 1.0, 1.0); }
    if (ckey==hlkeydown) { glColor4f(0.68f, 0.33f, 0.0f, 0.94f); }
    glVertex2f(2, y-j*kh); glVertex2f(2+kw, y-j*kh);
    glVertex2f(2+kw, y-(j+1)*kh); glVertex2f(2, y-(j+1)*kh);
    glEnd();
    glBegin(GL_LINE_LOOP);
    glColor4f(0.0, 0.0, 0.0, 0.4);
    glVertex2f(2, y-j*kh); glVertex2f(2+kw, y-j*kh);
    glVertex2f(2+kw, y-(j+1)*kh+1); glVertex2f(2, y-(j+1)*kh+1);
    glEnd();
    ckey+=wkj[j];
  }
  ckey=oct*12+1;
  for(i=0;i<5;i++) { // black keys
    j=i; if (i>1) j+=1;
    glBegin(GL_QUADS);
    if (ckey==hlkey) { glColor4f(0.28, 0.28, 0.28, 1.0);
    } else { glColor4f(0.0, 0.0, 0.0, 1.0); }
    if (ckey==hlkeydown) { glColor4f(0.68f, 0.33f, 0.0f, 0.94f); }
    glVertex2f(2, y-(j+0.5)*kh-(0.2*kh));
    glVertex2f(2+kw*0.6, y-(j+0.5)*kh-(0.2*kh));
    glVertex2f(2+kw*0.6, y-(j+1.5)*kh+(0.2*kh));
    glVertex2f(2, y-(j+1.5)*kh+(0.2*kh));
    glEnd();
    glBegin(GL_LINE_LOOP);
    glColor4f(1.0, 1.0, 1.0, 0.4);
    glVertex2f(2, y-(j+0.5)*kh-(0.2*kh));
    glVertex2f(2+kw*0.6, y-(j+0.5)*kh-(0.2*kh));
    glVertex2f(2+kw*0.6, y-(j+1.5)*kh+(0.2*kh));
    glVertex2f(2, y-(j+1.5)*kh+(0.2*kh));
    glEnd();
    ckey+=bkj[i];
  }
  sprintf(tmps, "c-%1d", oct);
  render_text(tmps, kw-15, y-2, 2, 0xff505050, 0);
}


// draw an octave of horizontal keyboard with upper-left at (x,y)
void draw_kbhoct(float x, float y, float kw, float kh, int oct, int hlkey, int hlkeydown, char *kbkeys)
{
  int i, j, ckey, ckb;
  char tmps[255];
  char wkj[]={2,2,1,2,2,2,1};
  char bkj[]={2,3,2,2};

  // draw an octave of keyboard
  ckey=oct*12;
  ckb=0;
  for (j=0;j<7;j++) { // white keys
    glBegin(GL_QUADS);
    if (ckey==hlkey) { glColor4f(1.0, 1.0, 1.0, 0.82);
    } else { glColor4f(1.0, 1.0, 1.0, 1.0); }
    if (ckey==hlkeydown) { glColor4f(0.68f, 0.33f, 0.0f, 0.94f); }
    glVertex2f(x+j*kw, y);
    glVertex2f(x+(j+1)*kw, y);
    glVertex2f(x+(j+1)*kw, y+kh);
    glVertex2f(x+j*kw, y+kh);
    glEnd();
    glBegin(GL_LINE_LOOP);
    glColor4f(0.0, 0.0, 0.0, 0.4);
    glVertex2f(x+j*kw, y);
    glVertex2f(x+(j+1)*kw, y);
    glVertex2f(x+(j+1)*kw, y+kh);
    glVertex2f(x+j*kw, y+kh);
    glEnd();
    
    if (kbkeys) { // && kbkeys[ckb] != '\0') {
      sprintf(tmps, "%1c", kbkeys[ckb]);
      render_text(tmps, x+j*kw+8, y+kh-4, 2, 0xff303030, 1);
      ckb+=wkj[j];
    }

    ckey+=wkj[j];
  }
  ckey=oct*12+1;
  ckb=1;

  for(i=0;i<5;i++) { // black keys
    j=i; if (i>1) j+=1;
    glBegin(GL_QUADS);
    if (ckey==hlkey) { glColor4f(0.28, 0.28, 0.28, 1.0);
    } else { glColor4f(0.0, 0.0, 0.0, 1.0); }
    if (ckey==hlkeydown) { glColor4f(0.68f, 0.33f, 0.0f, 0.94f); }
    glVertex2f(x+j*kw+(0.7*kw), y);
    glVertex2f(x+(j+1)*kw+(0.3*kw), y);
    glVertex2f(x+(j+1)*kw+(0.3*kw), y+(kh*0.5));
    glVertex2f(x+j*kw+(0.7*kw), y+(kh*0.5));
    glEnd();
    glBegin(GL_LINE_LOOP);
    glColor4f(1.0, 1.0, 1.0, 0.4);
    glVertex2f(x+j*kw+(0.7*kw), y);
    glVertex2f(x+(j+1)*kw+(0.3*kw), y);
    glVertex2f(x+(j+1)*kw+(0.3*kw), y+(kh*0.5));
    glVertex2f(x+j*kw+(0.7*kw), y+(kh*0.5));
    glEnd();

    if (kbkeys) { // && kbkeys[ckb] != '\0') {
      sprintf(tmps, "%1c", kbkeys[ckb]);
      render_text(tmps, x+j*kw+(0.7*kw)+5, y+0.5*kh-5, 2, 0xffc0c0c0, 1);
      ckb+=bkj[i];
    } else {
//      sprintf(tmps, "#");
//      render_text(tmps, x+j*kw+(0.7*kw)+2, y+0.5*kh-5, 2, 0xffc0c0c0, 0);
    }      


    ckey+=bkj[i];
  }
  sprintf(tmps, "%1d", oct);
//  render_text(tmps, x+1, y+kh-4, 2, 0xff505050, 0);
  render_text(tmps, x+1, y+kh+10, 2, 0xff909090, 0);
}





// pos = 0..1, slwidth=0..1
void draw_hslider(float x, float y, float width, float height, float pos, float onscreen, float total, int type)
{
  float slx, slw;

  // full background and frame
  glColor4f(0.1, 0.1, 0.1, 0.7);
  glBegin(GL_QUADS);
  glVertex2f(x, y);
  glVertex2f(x+width, y);
  glVertex2f(x+width, y+height);
  glVertex2f(x, y+height);
  glEnd(); 
  glColor4f(0.3, 0.3, 0.3, 0.7);
  glBegin(GL_LINE_LOOP);
  glVertex2f(x, y);
  glVertex2f(x+width, y);
  glVertex2f(x+width, y+height);
  glVertex2f(x, y+height);
  glEnd();
  
  // slider bit
  slw=width * (onscreen/total);
  if (slw>width) slw=width;
//  slx=x + (pos/total)*(width-slw);
  slx=x + (pos/(total-onscreen))*(width-slw);
  if (type) {
    glColor4f(0.2, 0.2, 0.2, 0.8);
  } else {
    glColor4f(0.15, 0.15, 0.15, 0.8);
  }
  glBegin(GL_QUADS);
  glVertex2f(slx, y);
  glVertex2f(slx+slw, y);
  glVertex2f(slx+slw, y+height);
  glVertex2f(slx, y+height);
  glEnd(); 
  glColor4f(0.5, 0.5, 0.5, 0.8);
  glBegin(GL_LINE_LOOP);
  glVertex2f(slx, y);
  glVertex2f(slx+slw, y);
  glVertex2f(slx+slw, y+height);
  glVertex2f(slx, y+height);
  glEnd(); 

  // gripper
  glColor4f(0.3, 0.3, 0.3, 0.8);
  glBegin(GL_LINES);
  glVertex2f(slx+(slw/2), y+2);
  glVertex2f(slx+(slw/2), y+height-2.5);
  glVertex2f(slx+(slw/2)-3, y+2);
  glVertex2f(slx+(slw/2)-3, y+height-2.5);
  glVertex2f(slx+(slw/2)+3, y+2);
  glVertex2f(slx+(slw/2)+3, y+height-2.5);
  glEnd();  
}

void draw_vslider(float x, float y, float width, float height, float pos, float onscreen, float total, int type)
{
  float sly, slh;

  // full background and frame
  glColor4f(0.1, 0.1, 0.1, 0.7);
  glBegin(GL_QUADS);
  glVertex2f(x, y);
  glVertex2f(x+width, y);
  glVertex2f(x+width, y+height);
  glVertex2f(x, y+height);
  glEnd(); 
  glColor4f(0.3, 0.3, 0.3, 0.7);
  glBegin(GL_LINE_LOOP);
  glVertex2f(x, y);
  glVertex2f(x+width, y);
  glVertex2f(x+width, y+height);
  glVertex2f(x, y+height);
  glEnd();
  
  // slider bit
  slh=height * (onscreen/total);
  if (slh>height) slh=height;
  sly=y + (pos/(total-onscreen))*(height-slh);
  if (type) {
    glColor4f(0.2, 0.2, 0.2, 0.8);
  } else {
    glColor4f(0.15, 0.15, 0.15, 0.8);
  }

  glBegin(GL_QUADS);
  glVertex2f(x, sly);
  glVertex2f(x+width, sly);
  glVertex2f(x+width, sly+slh);
  glVertex2f(x, sly+slh);
  glEnd(); 
  glColor4f(0.5, 0.5, 0.5, 0.8);
  glBegin(GL_LINE_LOOP);
  glVertex2f(x, sly);
  glVertex2f(x+width, sly);
  glVertex2f(x+width, sly+slh);
  glVertex2f(x, sly+slh);
  glEnd(); 

  // gripper
  glColor4f(0.3, 0.3, 0.3, 0.8);
  glBegin(GL_LINES);
  glVertex2f(x+2, sly+(slh/2));
  glVertex2f(x+width-2, sly+(slh/2));
  glVertex2f(x+2, sly+(slh/2)-3);
  glVertex2f(x+width-2, sly+(slh/2)-3);
  glVertex2f(x+2, sly+(slh/2)+3);
  glVertex2f(x+width-2, sly+(slh/2)+3);

  glEnd();  
}


// test if coordinates are inside a given box
int hovertest_box(float x, float y, float bx, float by, float height, float width)
{
  int hit= (
    (x > (bx-(width/2))) && (x < (bx+(width/2))) &&
    (y > (by-(height/2))) && (y < (by+(height/2)))
  );
  return hit;
}

int hovertest_hslider(float x, float y,
  float sx, float sy, float width, float height, float pos, float onscreen, float total)
{
  float slw, slx;
  int hit;

  slw=width * (onscreen/total);
  if (slw>width) slw=width;
  slx=sx + (pos/total)*(width-slw);
  hit=(
    ((x > slx) && (x < (slx+slw))) &&
    ((y > sy)  && (y < (sy+height)))
  );
  if (hit) return (x-slx);
  return 0.0;
}

int hovertest_vslider(float x, float y,
  float sx, float sy, float width, float height, float pos, float onscreen, float total)
{
  float slh, sly;
  int hit;

  slh=height * (onscreen/total);
  if (slh>height) slh=height;
//  sly=sy + (pos/total)*(height-slh);
  sly=sy + (pos/(total-onscreen))*(height-slh);
  hit=(
     (x > sx)   && (x < (sx+width)) &&
     (y > sly)  && (y < (sly+slh) )
  );
  if (hit) return 1;
  return 0;
}


void textbox_edit(char *text, unsigned char key, int size)
{
  //printf("key is %d\n", key);
  if (key==127 || key==8) // del or backspace
  { if (strlen(text)>0) text[strlen(text)-1]='\0'; }
  if ( ((key>=32 && key<127) /*|| (key>0xa0 && key<=0xff)*/) &&
       strlen(text)<size )
  { text[strlen(text)+1]='\0'; text[strlen(text)]=key; } 
}

