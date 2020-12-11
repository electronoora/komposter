/*
 * Komposter
 *
 * Copyright (c) 2010 Noora Halme et al. (see AUTHORS)
 *
 * This code is licensed under the GNU General Public
 * License version 2. See LICENSE for full text.
 *
 * File operations for IFF formats
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "constants.h"
#include "fileops.h"
#include "modules.h"
#include "patch.h"

/*
  The loaders don't look too closely at the data, so a chunk that
  is corrupted but with a proper header will most certainly crash
  the program.
*/

// from synthesizer.c
extern synthmodule mod[MAX_SYNTH][MAX_MODULES];
extern char synthname[MAX_SYNTH][128];
extern int signalfifo[MAX_SYNTH][MAX_MODULES];

// from patch.c
extern char patchname[MAX_SYNTH][MAX_PATCHES][128];
extern float modvalue[MAX_SYNTH][MAX_PATCHES][MAX_MODULES];
extern int modquantifier[MAX_SYNTH][MAX_PATCHES][MAX_MODULES];

// from pattern.c
extern u32 pattdata[MAX_PATTERN][MAX_PATTLENGTH];
extern u32 pattlen[MAX_PATTERN];

// fron sequencer.c
extern int seqch;
extern int seqsonglen;
extern int bpm;
extern int seq_synth[MAX_CHANNELS]; // which synth assigned to each channel
extern int seq_restart[MAX_CHANNELS]; // restart flags
extern int seq_pattern[MAX_CHANNELS][MAX_SONGLEN];
extern int seq_repeat[MAX_CHANNELS][MAX_SONGLEN];
extern int seq_transpose[MAX_CHANNELS][MAX_SONGLEN];
extern int seq_patch[MAX_CHANNELS][MAX_SONGLEN];



//
// song load and save functions
//
int load_ksong(char *filename)
{
  FILE *f;
  int chunklen, flen;
  char chunktype[4];
  int i, r, cpat, csyn;

  // clean up after the old song first

  f=fopen(filename, "rb");
  if (!f) {
    return FILE_ERROR_FOPEN;
  }
  chunklen=probe_chunk(f, chunktype);
  if (memcmp(chunktype, "KSNG", 4)) {
    return FILE_ERROR_CORRUPT;
  }

  fseek(f, 0, SEEK_END);
  flen=ftell(f);
  fseek(f, 0, SEEK_SET);
  if (flen != (chunklen+8)) {
    printf("err flen %d chunklen %d\n",flen,chunklen);
    return FILE_ERROR_CORRUPT;
  }
  fseek(f, 8, SEEK_SET);
  
  // ok, load the ksng chunk counts
  r=fread(&cpat, sizeof(int), 1, f);
  if (cpat<0 || cpat>MAX_PATTERN) {
    printf("cpat does not make sense %d\n", cpat);
    return FILE_ERROR_CORRUPT;
  }
  r=fread(&csyn, sizeof(int), 1, f);
  if (csyn<0 || cpat>MAX_SYNTH) {
    printf("csyn does not make sense %d\n", csyn);
    return FILE_ERROR_CORRUPT;
  }

  // load chunks
  r=load_chunk_kseq(f);
  for(i=0;i<cpat;i++) {
    r=load_chunk_kpat(i, f);
  }
  for(i=0;i<csyn;i++) {
    r=load_chunk_ksyn(i, f);
    r=load_chunk_kbnk(i, f);
  }
  
  // done
  return 0;
}


int save_ksong(char *filename)
{
  u32 crc;
  int datasize, t;
  int c_kpat, c_ksyn;
  FILE *f;
  int r, i, nm, n, m, mm;
  
  // calc number of patterns to save
  c_kpat=MAX_PATTERN;
  do {
    c_kpat--;
    for(n=0,i=0;i<pattlen[c_kpat]*16;i++) {
      if ((pattdata[c_kpat][i]&0xff)>0) n++;
    }
//    printf("pattern %d has %d note events\n", c_kpat, n);
    if (n) { c_kpat++; break; }
  } while (c_kpat>0);
//  printf("saving %d patterns\n", c_kpat);
  
  // calc number of synths to save
  c_ksyn=MAX_SYNTH;
  do {
    c_ksyn--;
    n=0;
    for(i=0;i<MAX_MODULES;i++) {
      if (mod[c_ksyn][i].type>=0) n++;
    }
//    printf("synth %d has %d modules\n", c_ksyn, n);
    if (n>3) { c_ksyn++; break; }
  } while (c_ksyn>0);
//  printf("saving %d synths+banks\n", c_ksyn);
  
  // calc chunk datasize
  datasize=8; // ksng data
  t=8 + 12 + 4*seqch + 4*seqch*seqsonglen*4;
//  printf("kseq %d (0x%x)\n", t,t);
  datasize+=t;
  
  for(i=0;i<c_kpat;i++) { 
    t=8+4+pattlen[i]*16*4; // kpat chunks
//    printf("kpat %d (0x%x)\n", t,t);
    datasize+=t;
  }
  
  for(i=0;i<c_ksyn;i++) { // ksyn and kbnk chunks
    nm=0;
    for(m=0;m<MAX_MODULES;m++) if (mod[i][m].type>=0) nm=m;
    nm++;
    t=8 + 8+128+nm*128; // ksyn
//    printf("ksyn %d (0x%x)\n", t,t);
    datasize+=t;
    
    m=0; mm=0;
    while(signalfifo[i][m]>=0) { if (signalfifo[i][m]>mm) mm=signalfifo[i][m]; m++; }
    mm++;
    t=8 + 8 + MAX_PATCHES * (128 + 3*mm*4); // kbnk
//    printf("kbnk %d (0x%x)\n", t,t);
    datasize+=t;
  }
  datasize+=4; // checksum
  
  f=fopen(filename, "wb");
  if (!f) return FILE_ERROR_FOPEN;

  r=fwrite("KSNG", sizeof(char), 4, f);
  r=fwrite(&datasize, sizeof(int), 1, f);

  r=fwrite(&c_kpat, sizeof(int), 1, f);
  r=fwrite(&c_ksyn, sizeof(int), 1, f);
  
  // save one kseq
  save_chunk_kseq(f);
  
  // save patterns
  for(i=0;i<c_kpat;i++) {
    save_chunk_kpat(i, f);
  }
  
  // save synths and patch banks
  for(i=0;i<c_ksyn;i++) {
    save_chunk_ksyn(i, f);
    save_chunk_kbnk(i, f);
  }

  // checksum
  crc=0; // TODO
  fwrite(&crc, sizeof(u32), 1, f);
  
  fclose(f);

  return 0;
}




//
// chunk load functions
//

int load_chunk_ksyn(int syn, FILE *f)
{
  unsigned char *chunkdata;
  int m, nm;

  // load chunk to ram
  chunkdata=load_chunk(f, "KSYN");
  if (!chunkdata) { return FILE_ERROR_CHUNKTYPE; }

  // looks ok, copy data from buffer
  memcpy(&nm, &chunkdata[0], sizeof(int));
  if (nm<0 || nm>MAX_MODULES) { free(chunkdata); return FILE_ERROR_CORRUPT; }

  memcpy(&synthname[syn], &chunkdata[8], 128);
  for(m=0;m<MAX_MODULES;m++) mod[syn][m].type=-1;
  memcpy(&mod[syn], &chunkdata[8+128], nm*128);
  synth_stackify(syn);
  
  free(chunkdata);
  return 0;
}


int load_chunk_kbnk(int syn, FILE *f)
{
  int p;
  long fpos;
  u32 np, sl;
  unsigned int sstk[MAX_MODULES];
  unsigned char *chunkdata;
  
  // load chunk to ram
  chunkdata=load_chunk(f, "KBNK");
  if (!chunkdata) { return FILE_ERROR_CHUNKTYPE; }

  // patch and module counts
  memcpy(&np, &chunkdata[0], sizeof(u32));
  memcpy(&sl, &chunkdata[4], sizeof(u32)); //stacklen

  // loop through patches
  for(p=0;p<np;p++) {
    fpos=8 + p * (128 + 3*sl*4);
    strncpy((char*)&patchname[syn][p], (char*)&chunkdata[fpos], 128); // patch name
    memcpy(&sstk, &chunkdata[fpos + 128], sl*4); // module indexes
    memcpy(&modvalue[syn][p],      &chunkdata[fpos+128+sl*4], sl*4);
    memcpy(&modquantifier[syn][p], &chunkdata[fpos+128+sl*8], sl*4);    
  }
  free(chunkdata);
  return 0;
}


int load_chunk_kpat(int patt, FILE *f)
{
  unsigned char *chunkdata;
  
  // load chunk to ram
  chunkdata=load_chunk(f, "KPAT");
  if (!chunkdata) { return FILE_ERROR_CHUNKTYPE; }

  memcpy(&pattlen[patt], &chunkdata[0], sizeof(unsigned int));
  memcpy(&pattdata[patt][0], &chunkdata[4], pattlen[patt]*16*4);
  free(chunkdata);
  return 0;
}


int load_chunk_kseq(FILE *f)
{
  int i;
  unsigned char *chunkdata;
  unsigned long filepos;
  
  // load chunk to ram
  chunkdata=load_chunk(f, "KSEQ");
  if (!chunkdata) { return FILE_ERROR_CHUNKTYPE; }

  memcpy(&seqch, &chunkdata[0], sizeof(int));
  memcpy(&seqsonglen, &chunkdata[4], sizeof(int));
  memcpy(&bpm, &chunkdata[8], sizeof(int));

  memcpy(seq_synth, &chunkdata[12], 4*seqch);
  for(i=0;i<seqch;i++) {
    seq_restart[i]=seq_synth[i]>>16;
    seq_synth[i]&=0xff;
  } // clean the flags out after memcpy
  
  filepos=12 + 4*seqch;
  for(i=0;i<seqch;i++) memcpy(seq_pattern[i], &chunkdata[filepos+i*seqsonglen*4], seqch*seqsonglen*4);  

  filepos+=seqch*seqsonglen*4;
  for(i=0;i<seqch;i++) memcpy(seq_repeat[i], &chunkdata[filepos+i*seqsonglen*4], seqch*seqsonglen*4);  

  filepos+=seqch*seqsonglen*4;
  for(i=0;i<seqch;i++) memcpy(seq_transpose[i], &chunkdata[filepos+i*seqsonglen*4], seqch*seqsonglen*4);  

  filepos+=seqch*seqsonglen*4;
  for(i=0;i<seqch;i++) memcpy(seq_patch[i], &chunkdata[filepos+i*seqsonglen*4], seqch*seqsonglen*4);  

  free(chunkdata);
  return 0;
}











//
// chunk save functions
//

int save_chunk_ksyn(int syn, FILE *f)
{
  unsigned char *filedata;
  unsigned int m, nm, dsize, pad;

  // what's the largest module number in use?
  nm=0;
  for(m=0;m<MAX_MODULES;m++) if (mod[syn][m].type>=0) nm=m;
  nm++;
  dsize=nm*sizeof(synthmodule) + 128 + 8;

  // build the file image in memory
  pad=0;
  filedata=calloc(dsize+8, sizeof(char));
  if (!filedata) { return 0; }
  memcpy(filedata,      "KSYN", 4);
  memcpy(&filedata[4],   &dsize, 4);
  memcpy(&filedata[8],   &nm, 4);
  memcpy(&filedata[12],  &pad, 4);
  memcpy(&filedata[16],  &synthname[syn], 128);
  for(m=0;m<nm;m++)
//    memcpy(&filedata[136+m*sizeof(synthmodule)], &mod[syn][m], sizeof(synthmodule));
    memcpy(&filedata[144+m*sizeof(synthmodule)], &mod[syn][m], sizeof(synthmodule));
  
  // save to disk
  errno=0;
  m=fwrite(filedata, sizeof(char), dsize+8, f);
  if (m!=(dsize+8)) { free(filedata); return FILE_ERROR_FWRITE; }
  free(filedata);

  // done
  return 0;
}


int save_chunk_kbnk(int syn, FILE *f)
{
  unsigned char *filedata;
  //char tmps[255];
  unsigned int p, m, mm, dsize, tmp;
  unsigned long fpos, stacklen;

  // what is the largest module id used in the signal stack?
  m=0; mm=0;
  while(signalfifo[syn][m]>=0) { if (signalfifo[syn][m]>mm) mm=signalfifo[syn][m]; m++; }
  stacklen=mm+1;
  
  // build the bank chunk image in memory
  dsize=8 + MAX_PATCHES * (128 + 3*stacklen*4);
  filedata=calloc(dsize+8, sizeof(char));
  if (!filedata) { return 0; }
  memcpy(filedata,      "KBNK", 4);
  memcpy(&filedata[4],   &dsize, 4);
  tmp=MAX_PATCHES;
  memcpy(&filedata[8],   &tmp, 4);
  memcpy(&filedata[12],  &stacklen, 4);

  fpos=16;
  for(p=0;p<MAX_PATCHES;p++) { // TODO: only save the patches actually used
    memcpy(&filedata[fpos],  &patchname[syn][p], 128);
    memcpy(&filedata[fpos+128], &signalfifo[syn], stacklen*4);
    memcpy(&filedata[fpos+128+stacklen*4], &modvalue[syn][p], stacklen*4);
    memcpy(&filedata[fpos+128+stacklen*8], &modquantifier[syn][p], stacklen*4);
    fpos+=128+stacklen*12;
  }

  // done, save chunk to file
  m=fwrite(filedata, sizeof(char), dsize+8, f);
  free(filedata);
  if (m!=fpos) { return FILE_ERROR_FWRITE; }
  return 0;
}



int save_chunk_kpat(int patt, FILE *f)
{
  int r;
  unsigned int chunklen;

  chunklen=4+pattlen[patt]*16*4;
  r=fwrite("KPAT", sizeof(char), 4, f);
  if (!r) { return FILE_ERROR_FWRITE; }
  r=fwrite(&chunklen, sizeof(unsigned int), 1, f);
  if (!r) { return FILE_ERROR_FWRITE; }
  r=fwrite(&pattlen[patt], sizeof(unsigned int), 1, f);
  if (!r) return FILE_ERROR_FWRITE;
  r=fwrite(&pattdata[patt], sizeof(unsigned int), pattlen[patt]*16, f);
  if (!r) { return FILE_ERROR_FWRITE; }
  return 0;
}


int save_chunk_kseq(FILE *f)
{
  // TODO: some checking on r - now any fwrite can fail and fuck things up nicely
  int r, i;
  unsigned int chunklen;

  chunklen=12 + 4*seqch + 4*seqch*seqsonglen*4;
  r=fwrite("KSEQ", sizeof(char), 4, f);
  r=fwrite(&chunklen, sizeof(unsigned int), 1, f);

  r=fwrite(&seqch, sizeof(unsigned int), 1, f);
  r=fwrite(&seqsonglen, sizeof(unsigned int), 1, f);
  r=fwrite(&bpm, sizeof(unsigned int), 1, f);

  for(i=0;i<seqch;i++) seq_synth[i]|=(seq_restart[i]&0xffff)<<16; // put restart flags to msw of synth number
  r=fwrite(seq_synth, sizeof(unsigned int), seqch, f);
  for(i=0;i<seqch;i++) seq_synth[i]&=0xff; // clean the flags out after fwrite
  
  for(i=0;i<seqch;i++) r=fwrite(seq_pattern[i],   sizeof(unsigned int), seqsonglen, f);  
  for(i=0;i<seqch;i++) r=fwrite(seq_repeat[i],    sizeof(unsigned int), seqsonglen, f);  
  for(i=0;i<seqch;i++) r=fwrite(seq_transpose[i], sizeof(unsigned int), seqsonglen, f);  
  for(i=0;i<seqch;i++) r=fwrite(seq_patch[i],     sizeof(unsigned int), seqsonglen, f);  

  return 0;
}



//
// utility functions
//

// loads a chunk of the given type from the file handle. returns
// a buffer with the content of the chunk. it is the duty of the
// caller to release the buffer once no longer needed.
void* load_chunk(FILE *f, char *chunktype)
{
  long filepos;
  int r;
  unsigned char fchunktype[4];
  u32 fchunklen;
  unsigned char *cbuffer;
  
  // check chunk type
  filepos=ftell(f);
  r=fread(fchunktype, sizeof(char), 4, f);
  if (!r) return NULL;
  r=fread(&fchunklen, sizeof(u32), 1, f);
  if (memcmp(fchunktype, chunktype, 4)) {
    fseek(f, filepos, SEEK_SET); 
    return NULL;
  }
  
  cbuffer=malloc(fchunklen);
  r=fread(cbuffer, sizeof(char), fchunklen, f);
  if (!r) {
    fseek(f, filepos, SEEK_SET);
    free(cbuffer);
    return NULL;
  }
  
  return cbuffer;
}


// checks the chunktype currently at the file pointer and
// stores it to the buffer pointed by chunktype
long probe_chunk(FILE *f, char *chunktype)
{
  long filepos;
  int r;
  u32 fchunklen;
  
  filepos=ftell(f);
  r=fread(chunktype, sizeof(char), 4, f);
  if (!r) { fseek(f, filepos, SEEK_SET); return 0; }
  r=fread(&fchunklen, sizeof(u32), 1, f);
  fseek(f, filepos, SEEK_SET);
  if (!r) { return 0; }
  return fchunklen;
}




//
// signal path following functions
//
void synth_stackify(int syn)
{
  int m, top;

  top=0;

  // clear fifo
  for(m=0;m<MAX_MODULES;m++) signalfifo[syn][m]=-1;
  
  // clear all tags first
  for(m=0;m<MAX_MODULES;m++) mod[syn][m].tag=0;
  
  // find the output module and start working backwards from it
  for(m=0;m<MAX_MODULES;m++)
    if (mod[syn][m].type==MOD_OUTPUT) { top=synth_trace(syn, m, top); break; }

  // set colors with a similar recursion
  synth_colorize(syn);

  // done - isn't recursion fun! :D
/*
  m=0;
  while(signalfifo[syn][m]>=0) {
    printf("%2d : %02d:%s\n",m,signalfifo[syn][m],
    modTypeNames[ mod[syn][ signalfifo[syn][m] ].type ]);
    m++;
  }
*/  
}
int synth_trace(int syn, int pm, int top)
{
  int n;

  // if this module is deleted or already tagged, return back
  if (pm<0) return top;
  if (mod[syn][pm].tag) return top;

  // tag this module and follow all patches which bring signals in  
  mod[syn][pm].tag=1;
  for(n=0;n<modInputCount[mod[syn][pm].type];n++)
    top=synth_trace(syn, mod[syn][pm].input[n], top);

  // push this module to the fifo and return
  signalfifo[syn][top++]=pm;
  return top;
}


void synth_colorize(int syn) {
  // recurse through the synth modules, "flow" the signal colors
  // downstream and set the effective output color for each module
  int m;

  // clear all tags first
  for(m=0;m<MAX_MODULES;m++) mod[syn][m].tag=0;

  // find a color for each module
  for(m=0;m<MAX_MODULES;m++) mod[syn][m].effective_color=synth_find_color(syn, m);
}
int synth_find_color(int syn, int pm) {
  int i, c, n;

  if (pm<0) return 0; // no color for invalid module index
  if (mod[syn][pm].tag) return mod[syn][pm].effective_color; // already visited, effective color should be set

  // find the color for this synth by finding the color of the upstream module
  mod[syn][pm].tag=1;
  
  if (mod[syn][pm].color==255) { // strip color if set to 255 on this node
    mod[syn][pm].effective_color=0; return 0;
  }
  
  if (mod[syn][pm].color || modInputCount[mod[syn][pm].type]==0) { // use the color of this node
    mod[syn][pm].effective_color=mod[syn][pm].color; return mod[syn][pm].effective_color;
  }
  
  // otherwise inherit color from upstream nodes
  c=0;  
  for (i=0;i<modInputCount[mod[syn][pm].type];i++) {
    n=synth_find_color(syn, mod[syn][pm].input[i]); 
    if (n && !c) { mod[syn][pm].effective_color=n; c=1; }
  }
  if (!c) mod[syn][pm].effective_color=0;
  return mod[syn][pm].effective_color;
}


void synth_update_bpm(int newbpm) {
  // check all synths and look for modules that have type MOD_KNOB or MOD_ATTENUATOR
  // if the parameter scale is SCALE_FREQUENCY_TEMPO or SCALE_DURATION_TEMPO, readjust
  // the effective value in all patches to match song bpm
  int s, m, p;
  float val;
    
  for(s=0;s<MAX_SYNTH;s++) {
    for(m=0;m<MAX_MODULES;m++) {
      if ((mod[s][m].type == MOD_KNOB || mod[s][m].type==MOD_ATTENUATOR) &&
          (mod[s][m].scale==SCALE_FREQUENCY_TEMPO || mod[s][m].scale==SCALE_DURATION_TEMPO)) {
        for(p=0;p<MAX_PATCHES;p++) {
          val=knob_scale2float(mod[s][m].scale, modvalue[s][p][m]); // un-scale raw float
          if (mod[s][m].scale==SCALE_FREQUENCY_TEMPO) {
            modvalue[s][p][m]=(val*60*OUTPUTFREQ)/newbpm; // re-scale frequency to raw with new bpm
	  } else {
            modvalue[s][p][m]=(OUTPUTFREQ*60)/(val*newbpm); // ditto with tempo
          }
        }
      }
    }
  }

  // done, activate new bpm
  bpm=newbpm;
}