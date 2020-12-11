/*
 * Komposter
 *
 * Copyright (c) 2010 Noora Halme et al. (see AUTHORS)
 *
 * This code is licensed under the GNU General Public
 * License version 2. See LICENSE for full text.
 *
 * Memory management for module buffers
 *
 */

#ifndef __BUFFERMM_H__
#define __BUFFERMM_H__

#include "modules.h"
#include "pattern.h"
#include "synthesizer.h"


#define KMM_ENTRIES     1024

void kmm_init(void);
void *kmm_alloc(unsigned long bytes, int voice, int synth, int module, int modtype);
void kmm_gcollect(void);

#endif
