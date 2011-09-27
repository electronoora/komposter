/*
 * Komposter (c) 2010-2011 Jani Halme (Firehawk/TDA)
 *
 * This code is licensed under the GNU General Public
 * License version 2. See file 'doc/LICENSE'.
 *
 * Architecture-specific include files
 *
 * $Rev$
 * $Date$
 */

// Apple Mac OS X
#ifdef __APPLE__
  #include <GLUT/glut.h>
  #include <OpenAL/al.h>
  #include <OpenAL/alc.h>
#else // BSD, Linux, etc.
  #include <GL/glut.h>
  #include <AL/al.h>
  #include <AL/alc.h>
#endif

