/*
 * Komposter - example converter from .ksong to asm source code
 *
 * (c) 2009-2010 Firehawk/TDA
 * 
 * This code is licensed under the MIT license:                             
 * http://www.opensource.org/licenses/mit-license.php
 *
 * Revision:    $Rev$
 * Last update: $Date$
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <math.h>

#include "constants.h"
#include "fileops.h"
#include "modules.h"


// from synthesizer.c
synthmodule mod[MAX_SYNTH][MAX_MODULES];
char synthname[MAX_SYNTH][128];
int signalfifo[MAX_SYNTH][MAX_MODULES];

// from patch.c
char patchname[MAX_SYNTH][MAX_PATCHES][128];
float modvalue[MAX_SYNTH][MAX_PATCHES][MAX_MODULES];
int modquantifier[MAX_SYNTH][MAX_PATCHES][MAX_MODULES];

// from pattern.c
unsigned long pattdata[MAX_PATTERN][MAX_PATTLENGTH];
unsigned int pattlen[MAX_PATTERN];

// from sequencer.c
int seqch;
int seqsonglen;
int bpm;
int seq_synth[MAX_CHANNELS]; // which synth assigned to each channel
int seq_restart[MAX_CHANNELS];
int seq_pattern[MAX_CHANNELS][MAX_SONGLEN];
int seq_repeat[MAX_CHANNELS][MAX_SONGLEN];
int seq_transpose[MAX_CHANNELS][MAX_SONGLEN];
int seq_patch[MAX_CHANNELS][MAX_SONGLEN];

// chunk counts from file
int patternct;
int synthct;
int patchct;

// data used during conversion
int synthlen[MAX_SYNTH];
int synthstart[MAX_SYNTH];
int patchstart[MAX_SYNTH*MAX_PATCHES];

int patternstart[MAX_CHANNELS];

int synthmap[MAX_SYNTH];
int patchmap[MAX_SYNTH*MAX_PATCHES];
int patternmap[MAX_PATTERN];


int delays=0; // how many delays are used simultaneously
int truesonglen;

// comparison function for sorting the sequencer events
int sqcompare(const void* va, const void* vb) {
  const int *a, *b;
  
  a=va; b=vb;
  if (*a>*b) return 1;
  if (*a<*b) return -1;
  return 0;
}

int main(int argc, char **argv) {
  int r, v, m, s, i, p, n, d, l, t, o;
  float f;
  unsigned int u;
  unsigned char sc;
  unsigned short note;

  // load the ksong file to memory
  if (argc!=2) {
    printf("komposter ksong converter (c) 2010 firehawk/tda\n\nusage:  %s <filename.ksong>\n\n", argv[0]);
    return -1;
  }
  r=load_ksong(argv[1]);
  if (r) { printf("Error loading .ksong! Errorcode %08x\n", r); return -1; }

  // song is now loaded to the data variables defined above. so if you want to
  // use your own player code, you'll need to modify the code below to suit your
  // needs. the example code i've included here uses my x86 assembly language
  // player which is provided as a reference implementation. the code is in nasm
  // syntax.
  //
  // this coverter will also strip any patterns and patches not actually used
  // from the output. this way the composer can send worktunes with additional
  // stuff in them and the coder can still get a good idea on how the tune will
  // actually compress.
  //
  // TODO: stripping unused synthesizers from the output - use synthmap to remap
  // the indexes. currently all synthesizsers in the ksong are converted.
  //

  // header
  printf(";; generated with komposter ksong-to-NASM converter (c) 2010 firehawk/tda\n;;\n");
  printf(";; source file:\n;;   %s\n;;\n", argv[1]);

  // gather a list of synthesizers used and map them to new indexes
  memset(&synthmap, -1, MAX_SYNTH*4);
  for(v=0;v<seqch;v++) {
    for(p=0;synthmap[p]>=0;p++) if (synthmap[p]==seq_synth[v]) break;
    if(synthmap[p]<0) synthmap[p]=seq_synth[v];
  }
  printf(";; synthesizers used:\n");
  for(i=0;synthmap[i]>=0;i++) printf(";;   %02x : (%02x:%-24s)\n",
    i, synthmap[i], synthname[synthmap[i]]);
  printf(";;\n");

  // before saving the patches and patterns, scan through the play sequence
  // to see which ones are actually used. they are stored into arrays and
  // the index into the array will be used to refer to them in the sequencer
  // eventlist
  memset(&patchmap, -1, MAX_SYNTH*MAX_PATCHES*4);
  memset(&patternmap, -1, MAX_PATTERN*4);
  for(v=0;v<seqch;v++) {
    for(i=0;i<seqsonglen;i++) {
      if (seq_pattern[v][i]>=0) {
        for(p=0;patternmap[p]>=0;p++) if (patternmap[p]==seq_pattern[v][i]) break;
        if (patternmap[p]<0) patternmap[p]=seq_pattern[v][i];
        u=seq_patch[v][i] | seq_synth[v]<<8; // patch is stored as synth<<8 | patch
        for(p=0;patchmap[p]>=0;p++) if (patchmap[p]==u) break;
        if (patchmap[p]<0) patchmap[p]=u;
      }
    }
  }
  printf(";; patterns used:\n;;   ");
  for(i=0;patternmap[i]>=0;i++) {
     printf("%02x ", patternmap[i]);
     if ((i&15)==15) printf("\n;;   ");
  }
  printf("\n;;\n;; patches used:\n");
  for(i=0;patchmap[i]>=0;i++) printf(";;   %02x : (%02x:%-24s) patch (%02x:%-24s)\n", 
    i, patchmap[i]>>8, synthname[patchmap[i]>>8], patchmap[i]&255, patchname[patchmap[i]>>8][patchmap[i]&255]);
  printf(";;\n;;\n\n");

  // find out the number of delay modules used during playback
  delays=0;
  for(v=0;v<seqch;v++) {
    s=seq_synth[v];
    m=0;
    while (signalfifo[s][m]>=0) {
      if (mod[s][signalfifo[s][m]].type==MOD_DELAY) delays++;
      m++;
    }
  }

  // find the actual length of the song, ie. end of last pattern on seq
  for(i=0,m=0;i<seqsonglen;i++) for(v=0;v<seqch;v++) {
     if (seq_pattern[v][i]>=0) {
       n=i+seq_repeat[v][i]*pattlen[seq_pattern[v][i]];
       if (n>m) m=n;
     }
  }
  truesonglen=m;

  // number of channels and synthesizers
  printf("%%define NUM_CHANNELS %d\n", seqch);
  printf("%%define NUM_SYNTHS %d\n", synthct);
  printf("%%define NUM_DELAYS %d\n", delays);
  printf("%%define SONG_LEN %d\n", truesonglen*16);
  printf("\n");

  // bpm rate converted to a tick divider (max. 255)
  t=OUTPUTFREQ/(bpm*256/60);
  printf("\n; bpm %d at %dhz sample rate\ntickdivider dd %05xh\n\n",bpm,OUTPUTFREQ,t);

  // master volume multiplier
  printf("; master volume is %f\nsamplemul dd %f\n\n", 1.0, 32766.0);
 
  // stackify each synth prior to converting and populate the fifopos member on each struct
  for(s=0;s<synthct;s++) {
    synth_stackify(s);
    m=0;
    while (signalfifo[s][m]>=0) {
      mod[s][signalfifo[s][m]].fifopos=m;
      m++;
    }    
  }
 
  // synth module types
  i=1;
  printf("modtypes: ; type of each module\n"); //\tdb 04h ; dummy zero cv\n");
  for(s=0;s<synthct;s++) {
    printf("\t; synth %02x\n\tdb 004h\n\tdb ", s);
    m=0;
    while (signalfifo[s][m]>=0) {
      if (m&7) printf(", ");
      printf("%03xh", mod[s][signalfifo[s][m]].type);
      if ((m&7)==7) printf("\n\tdb ");
      m++;
    }
    printf("\n");
    synthlen[s]=m;
    i+=m;
  }
  printf("\n");

  // synth module inputs
  i=0;
  printf("modinputs: ; signal inputs for each module\n");
  for(s=0;s<synthct;s++) {   
    synthstart[s]=i;
    printf("\t; synth %03x\n\tdb 000h, 000h, 000h, 000h ; %02x: dummy zero\n", s, i++);
    m=0;
    while (signalfifo[s][m]>=0) {
      for(r=0;r<4;r++) {
        // get the fifo position instead
        if (mod[s][signalfifo[s][m]].input[r]>=0) {
          // replace the input module index with its fifo position plus one to account for the dummy zero
          mod[s][signalfifo[s][m]].input[r]=mod[s][ mod[s][signalfifo[s][m]].input[r] ].fifopos + 1;
        } else {
          mod[s][signalfifo[s][m]].input[r]=0; // unconnected, feed zero from the dummy
        }

      }
      // inputs are written in reverse order so that they end up the
      // right way when loaded as a single dword on small-endian architecture
      printf("\tdb %03xh, %03xh, %03xh, %03xh ; %02x: %02x %s\n", 
        mod[s][signalfifo[s][m]].input[3], mod[s][signalfifo[s][m]].input[2],
        mod[s][signalfifo[s][m]].input[1], mod[s][signalfifo[s][m]].input[0],
        i+m, m+1, modTypeNames[mod[s][signalfifo[s][m]].type]);
      m++;
    }
    i+=m;
    printf("\n");
  }

  // offsets to module data for the start of each synthesizer
  printf("synthstart: ; start offset for each synth\n\tdw ");
  for(s=0;s<synthct;s++) {
    if (s) printf(", ");
    printf("%05xh", synthstart[s]);
  }
  printf("\n\n");

  // output the patch modulator data
  r=0;
  printf("patchdata: ; modulator data for all patches\n");
  for(i=0;patchmap[i]>=0;i++) {
    patchstart[i]=r;
    s=patchmap[i]>>8;
    p=patchmap[i]&255;
    printf("\t; %02d: synth %d patch %d\n\tdd 0.00, ",i,s,p);
    r++;
    m=0;
    while (signalfifo[s][m]>=0) {
      if (m&3) printf(", ");
      t=mod[s][signalfifo[s][m]].type;
      switch (modModulatorTypes[t]) {
        // 0=no modulator, 1=float, 2=integer
        case 0:
          printf( "0h" );
          break;
        case 1:
          f=modvalue[s][p][signalfifo[s][m]];
          if ((f-floor(f))>0) {
            printf("%10.10g", f);
          } else {
            printf("%10.10f", f);
          }
          break;
        case 4: // lfo, 0=triangle, 1=sine
          if ( (int)(modvalue[s][p][signalfifo[s][m]]) == 3) {
            printf( "01h" );
          } else {
            printf( "00h" );
          }
          break;
        case 5: // filter
        default:
          printf( "0%xh", (int)(modvalue[s][p][signalfifo[s][m]]) );
          break;
      }
      if ((m&3)==3) printf("\n\tdd ");
      m++;
    }
    r+=m;
    printf("\n");
  }
  printf("\n");
 
  // offsets to start of each patch
  printf("patchstart: ; start offset for each patch\n\tdw ");
  for(i=0;patchmap[i]>=0;i++) {
    if (i&15) printf(", ");
    printf("%05xh", patchstart[i]);
    if ((i&15)==15) printf("\n\tdw ");
  }
  printf("\n\n");

  // synthesizer number used on each channel
  printf("seqvoice: ; synth used on each channel\n\tdb ");
  for(v=0;v<(seqch-1);v++) printf("%03xh, ", seq_synth[v]);
  printf("%03xh\n\n", seq_synth[seqch-1]);

  // seq restart flags
  printf("seqmask: ; mask for channel flags\n\tdb ");
  for(v=0;v<(seqch-1);v++) {
    printf("0x%02x, ", (unsigned char)((seq_restart[v])<<4)|1);
  }
  printf("0x%02x\n\n", (unsigned char)((seq_restart[v])<<4)|1);

  // output the raw note on/off data with lots of zeroes in between. :)
  printf("songdata:\n");
  for(v=0;v<seqch;v++) {
    printf("\t;ch%02d\n\tdw ", v);
    patternstart[v]=o;
    
    int restcount=0;
    int patternrepeats=0;
    int patterntranspose=0;
    int noteon=0;


    p=0;
    while (p<truesonglen) {

      if (seq_pattern[v][p]>=0) {
        // a new pattern on the sequence
        i=seq_pattern[v][p]; // pattern index
        patternrepeats = seq_repeat[v][p];
        patterntranspose = seq_transpose[v][p];
        int patch=0;
        for(m=0;patchmap[m]>=0;m++) 
          if (
               ( (patchmap[m]&255)==seq_patch[v][p]  ) && 
               ( (patchmap[m]>>8)==seq_synth[v]      )
             ) patch=m+1;

        // i = pattern index
        // scan through the pattern and generate notes and rests
        // increment r on each byte written
        for(r=0;r<patternrepeats;r++) {
          for(n=0; n<(pattlen[i]*16); n++) {
            note=patch<<8;
            patch=0;
            if (pattdata[i][n] > 0) {
              d=pattdata[i][n] & 0x7f;
              if (noteon != d) {
                note|=(d+patterntranspose); //|0x80;
                if (pattdata[i][n] & NOTE_ACCENT) note|=0x4000;
                noteon=d;
              }
              if (!(pattdata[i][n+1]&NOTE_LEGATO)) { note|=0x8000; noteon=0; }
            }
            printf("0x%04x, ", note);
            if ((n&15)==15) { printf(" ; %d\n\tdw ", p); p++; }
          }
        }
      } else {
        // no pattern here, add rest for 1 measure
        printf("0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000 ; %d\n\tdw ",p);
        p++;
      }
    }
    printf("\n\t; p=%d\n\n", p);
    
  }
  printf("\n");





  



    











  
  // footer
  printf(";; eof\n");
  return 0;
}