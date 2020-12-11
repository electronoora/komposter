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

#ifndef __FILEOPS_H__
#define __FILEOPS_H__


// error codes from chunk loaders
#define		FILE_ERROR_FWRITE	1
#define		FILE_ERROR_FREAD	2
#define		FILE_ERROR_CHUNKTYPE	3
#define		FILE_ERROR_FOPEN	4
#define		FILE_ERROR_CORRUPT	5


//
// file load and save functions
//
int load_ksong(char *filename);
int save_ksong(char *filename);



//
// chunk load functions
//
int load_chunk_ksyn(int syn, FILE *f);
int load_chunk_kbnk(int syn, FILE *f);
int load_chunk_kpat(int patt, FILE *f);
int load_chunk_kseq(FILE *f);



//
// chunk save functions
//
int save_chunk_ksyn(int syn, FILE *f);
int save_chunk_kbnk(int syn, FILE *f);
int save_chunk_kpat(int patt, FILE *f);
int save_chunk_kseq(FILE *f);


//
// utility functions
//
void* load_chunk(FILE *f, char *chunktype);
long probe_chunk(FILE *f, char *chunktype);


//
// signal path following functions
//
void synth_stackify(int syn);
int synth_trace(int syn, int pm, int top);
void synth_colorize(int syn);
int synth_find_color(int syn, int pm);
void synth_update_bpm(int newbpm);

#endif
