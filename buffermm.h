/*
 * Komposter (c) 2010-2011 Jani Halme (Firehawk/TDA)
 *
 * This code is licensed under the GNU General Public
 * License version 2. See file 'doc/LICENSE'.
 *
 * Memory management for module buffers
 *
 * $Rev$
 * $Date$
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
