/*
 * Komposter
 *
 * Copyright (c) 2010 Noora Halme et al. (see AUTHORS)
 *
 * This code is licensed under the GNU General Public
 * License version 2. See LICENSE for full text.
 *
 * Shader loading and compiling
 *
 */

#ifndef __SHADER_H__
#define __SHADER_H__

#include <string.h>
#include "arch.h"

// number of shaders
#define SHADER_COUNT 1

void programerrors(GLuint pr);
int shader_init(void);

#endif
