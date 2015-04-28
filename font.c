/*
 * Komposter (c) 2010-2011 Jani Halme (Firehawk/TDA)
 *
 * This code is licensed under the GNU General Public
 * License version 2. See file 'doc/LICENSE'.
 *
 * Font loading, pre-rendering and drawing to display
 *
 * $Rev$
 * $Date$
 */

#include "font.h"

// number of font styles specified
#define FONT_STYLES 8

// from main.c
extern char respath[512];

// eight font styles
char fontfile[FONT_STYLES][255]={
  "m42.TTF",		    // komposter title font
  "",
  "acknowtt.ttf",             // tiny pixel font for modules
  "078MKSD_MC.TTF",           // module descriptions
  "",
  "",
  "",
  "",
};
int fontsize[FONT_STYLES]={
  8,
  0,
  12,
  16,
  0,
  0,
  0,
  0
};


// freetype library struct and font face structs
FT_Library ft;
FT_Face font[FONT_STYLES];

// data for prerendered font bitmaps
int font_advance[FONT_STYLES][255];
int font_xoffset[FONT_STYLES][255];
int font_yoffset[FONT_STYLES][255];
int font_width[FONT_STYLES][255];
int font_height[FONT_STYLES][255];
float font_ucoord[FONT_STYLES][255];
float font_vcoord[FONT_STYLES][255];
GLuint font_texture[FONT_STYLES][255];
unsigned long *font_bitmap[FONT_STYLES][255];


// round a 32-bit value upward to nearest power of 2
unsigned int tpow2(unsigned int x)
{ x--;x|=x>>1;x|=x>>2;x|=x>>4;x|=x>>8;x|=x>>16;x++;return x; }


// initialize freetype, load font faces and prerender the font
// glyphs with sizes specified in the global font settings
int font_init(void)
{
  char fullpath[512];
  int err, i, j, c, rc, f, texw, texh;
  unsigned long p;
  FT_GlyphSlot slot;

  // init freetype
  err=FT_Init_FreeType(&ft);
  if (err) {
    printf("Freetype error on FT_Init_FreeType()\n");
    return 0;
  }

  // load font faces and set sizes
  for(i=0;i<FONT_STYLES;i++) {
    strncpy(fullpath, respath, 511);
    strncat(fullpath, fontfile[i], 511);
    if (fontsize[i] > 0) {
//      err=FT_New_Face(ft, fontfile[i], 0, &font[i]); 
      err=FT_New_Face(ft, fullpath, 0, &font[i]); 
      if (err) {
        printf("Freetype error on FT_New_Face(), path %s\n", fullpath);
        return 0;
      }
      err=FT_Set_Pixel_Sizes(font[i], 0, fontsize[i]);
    }
  }
  
  // prerender all glyphs
  for(f=0;f<FONT_STYLES;f++) {
    if (fontsize[f] > 0) {
      slot=font[f]->glyph;
      for(c=0;c<255;c++) {
        // awful kludge to fix W and M with the bitmap font
        rc=c;
        if (c=='W') rc='w';
        if (c=='M') rc='m';
        
        // render character
        err=FT_Load_Char(font[f], rc, FT_LOAD_RENDER);
        if (err) {
          printf("Freetype error on FT_Load_Char()\n");
          continue;
        }

        // save metrics
        font_advance[f][c]=slot->advance.x >> 6;
        font_xoffset[f][c]=slot->bitmap_left;
        font_yoffset[f][c]=-slot->bitmap_top;
        font_width[f][c]=slot->bitmap.width;
        font_height[f][c]=slot->bitmap.rows;
        texw=tpow2(slot->bitmap.width);
        texh=tpow2(slot->bitmap.rows);
        font_ucoord[f][c]=(float)(slot->bitmap.width)/(float)(texw);
        font_vcoord[f][c]=(float)(slot->bitmap.rows)/(float)(texh);

        // alloc ram for bimap and convert to RGBA
        font_bitmap[f][c]=calloc(texw*texh, 4);
        for (j=0;j<slot->bitmap.rows;j++)
        {
          for(i=0;i<slot->bitmap.width;i++) {
            p= 0xffffff|((unsigned long)(slot->bitmap.buffer[j*slot->bitmap.pitch+i]) << 24);
            font_bitmap[f][c][ ( slot->bitmap.rows-(j+1) )*texw + i] = p;
          }
        }

        // generate opengl texture for the bitmap
        glEnable(GL_TEXTURE_2D);
        glGenTextures(1, &font_texture[f][c]);
        glBindTexture(GL_TEXTURE_2D, font_texture[f][c]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texw, texh, 0, GL_RGBA, GL_UNSIGNED_BYTE, font_bitmap[f][c]);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
        glDisable(GL_TEXTURE_2D);
      }
    }
  }
  
  return 1;
}




// render text using a preloaded face/size
// fontnr is preloaded face/size number, color is argb
// align 0=left, 1=center, 2=right
void render_text(char *text, float x, float y, int fontnr, unsigned long color, int align)
{
  unsigned long sw, n;
  float xp, yp;

  // calc text bitmap width (note: linefeeds fuck up center and right alignment)
  sw=0; n=0;
  while(n<strlen(text))
  {
    sw+=font_advance[fontnr][(int)text[n]];
    n++;
  }

  // render the string using textured quads
  xp=x;
  if (align==1) xp-=(sw/2);
  if (align==2) xp-=sw;
  yp=y; n=0;
  while(n<strlen(text))
  {
    if ((int)text[n]=='\n') { yp+=fontsize[fontnr]+2; xp=x; n++; continue; }
    glEnable(GL_TEXTURE_2D);
    glColor4ub((color>>16)&0xff, (color>>8)&0xff, color&0xff, color>>24);
    glBindTexture(GL_TEXTURE_2D, font_texture[fontnr][(int)text[n]]);
    glBegin(GL_QUADS);
      glTexCoord2f(0, font_vcoord[fontnr][(int)text[n]]);
      glVertex2f(xp+font_xoffset[fontnr][(int)text[n]], yp+font_yoffset[fontnr][(int)text[n]]);
      glTexCoord2f(font_ucoord[fontnr][(int)text[n]], font_vcoord[fontnr][(int)text[n]]);
      glVertex2f(xp+font_xoffset[fontnr][(int)text[n]]+font_width[fontnr][(int)text[n]], yp+font_yoffset[fontnr][(int)text[n]]);
      glTexCoord2f(font_ucoord[fontnr][(int)text[n]], 0);
      glVertex2f(xp+font_xoffset[fontnr][(int)text[n]]+font_width[fontnr][(int)text[n]],
               yp+font_yoffset[fontnr][(int)text[n]]+font_height[fontnr][(int)text[n]]);
      glTexCoord2f(0, 0);
      glVertex2f(xp+font_xoffset[fontnr][(int)text[n]],
               yp+font_yoffset[fontnr][(int)text[n]]+font_height[fontnr][(int)text[n]]);
    glEnd();
    glDisable(GL_TEXTURE_2D);
    
    xp+=font_advance[fontnr][(int)text[n]];
    n++;
  }
}

