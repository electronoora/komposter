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

#include "buffermm.h"

/*
  memory management for keeping count on what buffers have been allocated and if they are still in use

  eg. delay module. when the buffer is no longer needed and must be freed?
  
  1. the module is deleted from the synthesizer -> all channels using the synth must release the memory
  2. the synthesizer on a channel is changed -> this channel must release the memory
  3. a new synthesizer is loaded from disk (see first point) 

  a garbage collector will scan the whole entry table and check that all parameters recorded match those
  currently active. with cases above;
  
  1/3. modtype will not match with mod-array -> synth has changed. release the buffer and set localdata for this voice to zero
  2. synth will not match with seq_synth -> channel uses different synth. release and set localdata to zero.

  localdata must be initialized to zero or there will be invalid pointer references left and right! also, when
  deleting modules or changing/loading synths, the localdata for all involved voices must be reset to zero. the
  garbage collector takes care of this, so calling it after each operation should keep things in check.

  the buffer pointer must be stored on the first dword in the localdata on each module which uses buffers!

  all in all - this is a pretty big mess and there has to be a better way to handle this. :)

*/


// from audio.c
extern float localdata[MAX_CHANNELS][MAX_MODULES][16];

// from synthesizer.c
extern synthmodule mod[MAX_SYNTH][MAX_MODULES];

// from sequencer.c
extern int seq_synth[MAX_CHANNELS]; // which synth assigned to each channel




typedef struct {
  void *ptr;
  unsigned long bytes;
  int voice;
  int synth;
  int module;
  int modtype;
} kmm_mement;


kmm_mement kmmtable[KMM_ENTRIES];


void kmm_init(void)
{
  int i;
  for(i=0;i<KMM_ENTRIES;i++) {
    kmmtable[i].ptr=NULL;
    kmmtable[i].bytes=0;
    kmmtable[i].voice=0xfe;
    kmmtable[i].synth=0xfe;
    kmmtable[i].module=0xfe;
    kmmtable[i].modtype=0xfe;
  }
}


void *kmm_alloc(unsigned long len, int voice, int synth, int module, int modtype)
{
  int i;
  void *buffer;
  
//  buffer=calloc(len, sizeof(unsigned long));
  buffer=malloc(len*4);
  if (buffer) {
    for(i=0;i<KMM_ENTRIES;i++) {
      if (!kmmtable[i].ptr) {
        kmmtable[i].ptr=buffer;
        kmmtable[i].bytes=len*sizeof(unsigned long);
        kmmtable[i].voice=voice;
        kmmtable[i].synth=synth;
        kmmtable[i].module=module;
        kmmtable[i].modtype=modtype;

        printf("kmm: module data buffer allocated from %08lx (v %d s %d mi %d mt %d)\n", (unsigned long)buffer,voice,synth,module,modtype);
        
        return buffer;
      }
    }
  }
  return 0; // table is full
}



void kmm_gcollect(void)
{
  int i;

  // scan the kmm table and free any buffers which are no longer being used
  for(i=0;i<KMM_ENTRIES;i++) {
    if (kmmtable[i].ptr) {
      if (seq_synth[kmmtable[i].voice] != kmmtable[i].synth) {
        // channel no longer uses this synth -> release the buffer
//        printf("kmm: synth changed, releasing module data from %08lx\n", (unsigned long)kmmtable[i].ptr);
printf("kmm: synth changed, releasing module data from %08lx (v %d s %d mi %d mt %d)\n",
  (unsigned long)kmmtable[i].ptr, kmmtable[i].voice, kmmtable[i].synth, kmmtable[i].module, kmmtable[i].modtype);
        free(kmmtable[i].ptr);
//        memset(&localdata[kmmtable[i].voice][kmmtable[i].module][0], 0, sizeof(void*));
        localdata[kmmtable[i].voice][kmmtable[i].module][0]=0;
        kmmtable[i].ptr=NULL;
        continue;
      }

      if (kmmtable[i].modtype != mod[kmmtable[i].synth][kmmtable[i].module].type) {
        // module type has changed or it has been deleted -> release
//        printf("kmm: modtype changed, releasing module data from %08lx\n", (unsigned long)kmmtable[i].ptr);
printf("kmm: modtype changed, releasing module data from %08lx (v %d s %d mi %d mt %d)\n",
  (unsigned long)kmmtable[i].ptr, kmmtable[i].voice, kmmtable[i].synth, kmmtable[i].module, kmmtable[i].modtype);
        free(kmmtable[i].ptr);
//        memset(&localdata[kmmtable[i].voice][kmmtable[i].module][0], 0, sizeof(void*));
        localdata[kmmtable[i].voice][kmmtable[i].module][0]=0;
        kmmtable[i].ptr=NULL;
        continue;
      }
    }
  }
}