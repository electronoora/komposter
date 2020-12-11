/*
 * Komposter converter
 *
 * Copyright (c) 2010 Noora Halme et al. (see AUTHORS)
 *
 * This code is licensed under the MIT license:                               
 * http://www.opensource.org/licenses/mit-license.php  
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "constants.h"
#include "fileops.h"
#include "modules.h"



// external data from converter.c
extern synthmodule mod[MAX_SYNTH][MAX_MODULES];
extern char synthname[MAX_SYNTH][128];
extern int signalfifo[MAX_SYNTH][MAX_MODULES];
extern char patchname[MAX_SYNTH][MAX_PATCHES][128];
extern float modvalue[MAX_SYNTH][MAX_PATCHES][MAX_MODULES];
extern int modquantifier[MAX_SYNTH][MAX_PATCHES][MAX_MODULES];
extern unsigned long pattdata[MAX_PATTERN][MAX_PATTLENGTH];
extern unsigned int pattlen[MAX_PATTERN];
extern int seqch;
extern int seqsonglen;
extern int bpm;
extern int seq_synth[MAX_CHANNELS]; // which synth assigned to each channel
extern int seq_restart[MAX_CHANNELS]; // channel restart flags
extern int seq_pattern[MAX_CHANNELS][MAX_SONGLEN];
extern int seq_repeat[MAX_CHANNELS][MAX_SONGLEN];
extern int seq_transpose[MAX_CHANNELS][MAX_SONGLEN];
extern int seq_patch[MAX_CHANNELS][MAX_SONGLEN];
extern int patternct;
extern int synthct; 
extern int patchct; 




// loads a .ksong file specified by the filename. returns 0 if everything went
// swimmingly, otherwise nonzero (see fileops.h for error codes).
int load_ksong(char *filename)
{
  FILE *f;
  int chunklen, flen;
  char chunktype[4];
  int i, r, cpat, csyn;

  // clean up after the old song first

  f=fopen(filename, "r");
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
  patternct=cpat;
  synthct=csyn;

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


// loads a ksyn chunk from the file handle as the specified synthesizer
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


// loads a kbnk patchbank chunk from the file handle for the specified synthesizer
int load_chunk_kbnk(int syn, FILE *f)
{
  int p;
  long fpos;
  unsigned int np, sl;
  unsigned int sstk[MAX_MODULES];
  unsigned char *chunkdata;
  
  // load chunk to ram
  chunkdata=load_chunk(f, "KBNK");
  if (!chunkdata) { return FILE_ERROR_CHUNKTYPE; }

  // patch and module counts
  memcpy(&np, &chunkdata[0], sizeof(unsigned int));
  memcpy(&sl, &chunkdata[4], sizeof(unsigned int)); //stacklen

  // loop through patches
  for(p=0;p<np;p++) {
    fpos=8 + p * (128 + 3*sl*4);
    strncpy((char*)&patchname[syn][p], (const char *)&chunkdata[fpos], 128); // patch name
    memcpy(&sstk, &chunkdata[fpos + 128], sl*4); // module indexes
    memcpy(&modvalue[syn][p],      &chunkdata[fpos+128+sl*4], sl*4);
    memcpy(&modquantifier[syn][p], &chunkdata[fpos+128+sl*8], sl*4);    
  }
  free(chunkdata);
  return 0;
}


// loads a kpat data chunk from the file handle as the specified pattern number
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


// loads a kseq data chunk from the file handle
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


// loads a chunk of the given type from the file handle. returns
// a buffer with the content of the chunk. it is the duty of the
// caller to release the buffer once no longer needed.
void* load_chunk(FILE *f, char *chunktype)
{
  long filepos;
  int r;
  unsigned char fchunktype[4];
  unsigned long fchunklen;
  unsigned char *cbuffer;
  
  // check chunk type
  filepos=ftell(f);
  r=fread(fchunktype, sizeof(char), 4, f);
  if (!r) return NULL;
  r=fread(&fchunklen, sizeof(long), 1, f);
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
  unsigned long fchunklen;
  
  filepos=ftell(f);
  r=fread(chunktype, sizeof(char), 4, f);
  if (!r) { fseek(f, filepos, SEEK_SET); return 0; }
  r=fread(&fchunklen, sizeof(long), 1, f);
  fseek(f, filepos, SEEK_SET);
  if (!r) { return 0; }
  return fchunklen;
}


// traverses the module tree and orders them into a fifo stack
// according to signal path dependencies. processing the modules
// in the stack order ensures that each module receives up to date
// input data.
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

  // done - isn't recursion fun! :D
}


// the actual tree traversing function
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
