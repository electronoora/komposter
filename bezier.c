/*
 * Komposter
 *
 * Copyright (c) 2010 Noora Halme et al. (see AUTHORS)
 *
 * This code is licensed under the GNU General Public
 * License version 2. See LICENSE for full text.
 *
 * Bezier curves
 *
 */

#include "bezier.h"

// cubic bezier curve function, writes 2d coordinates to pt
// t=[0, 1.0]
pt *bezier(pt *p, bzr *bz, float t) {
  float i, a, b, c, d;

  // B(t) = (1-t)^3 * p0 + 3(1-t)^2*t*c0 + 3(1-t)*t^2*c1 + t^3 * p1
  i=1.0-t;
  a=i*i*i;
  b=3*i*i*t;
  c=3*i*t*t;
  d=t*t*t;

  p->x=a * bz->x0 + b * bz->cx0 + c * bz->cx1 + d * bz->x1;
  p->y=a * bz->y0 + b * bz->cy0 + c * bz->cy1 + d * bz->y1;
  
  return p;
}

