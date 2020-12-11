/*
 * Komposter
 *
 * Copyright (c) 2010 Noora Halme et al. (see AUTHORS)
 *
 * This code is licensed under the GNU General Public
 * License version 2. See LICENSE for full text.
 *
 * Architecture-specific include files
 *
 */


// dword size depending on platform
#if __x86_64__
  #define u32 unsigned int
  #define s32 int
#else
  #define u32 unsigned long
  #define s32 long
#endif


// Apple Mac OS X
#ifdef __APPLE__
  #include <GLUT/glut.h>
  #include <OpenAL/al.h>
  #include <OpenAL/alc.h>
#else // BSD, Linux, etc.
  // to define functions past OpenGL 1.3
  #define GL_GLEXT_PROTOTYPES

  #include <GL/gl.h>
  #include <GL/glext.h>
  #include <GL/glut.h>
  #include <AL/al.h>
  #include <AL/alc.h>
#endif

