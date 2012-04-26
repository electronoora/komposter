/*
 * Komposter (c) 2010-2011 Jani Halme (Firehawk/TDA)
 *
 * This code is licensed under the GNU General Public
 * License version 2. See file 'doc/LICENSE'.
 *
 * Bezier curves
 *
 * $Rev$
 * $Date$
 */

#ifndef __BEZIER_H__
#define __BEZIER_H__

#include <math.h>

typedef struct {
  float x, y;
} pt;

typedef struct {
  float x0, y0;
  float x1, y1;
  float cx0, cy0;
  float cx1, cy1;

  float a;
} bzr;

pt *bezier(pt *p, bzr *bz, float t);

#endif
