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

#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include "shader.h"

// from main.c
extern char respath[512];

// shader file names
char shaderfile[SHADER_COUNT][255]={
  "palette.frag"
};

// handles for shader programs
GLuint shader_program[SHADER_COUNT];



// print shader compile errors
void programerrors(GLuint pr) {
  int maxlen, infolen;
  char info[65536];

  glGetProgramiv(pr,GL_INFO_LOG_LENGTH,&maxlen);
  glGetProgramInfoLog(pr, maxlen, &infolen, info);
  printf("%s\n",info);
}


// load shader from file and compile, returns handle
GLuint loadshader(char *fname, GLuint type)
{
  char *temp;
  GLuint sh;
  FILE *f;
  int maxlen, infolen;
  char info[65536];
fprintf(stderr, "loading shader %s\n", fname); 
  temp=calloc(65536, 1);
  f=fopen(fname, "r");
  fread(temp, 1, 65536, f);
  fclose(f);
  sh=glCreateShader(type);
  glShaderSource(sh, 1, (const GLchar* const *)&temp, NULL);
  glCompileShader(sh);
  if(glIsShader(sh)) {
    glGetShaderiv(sh, GL_INFO_LOG_LENGTH, &maxlen);
    glGetShaderInfoLog(sh, maxlen, &infolen, info);
    if (infolen>0) printf("%s\n", info);
  }
  return sh;
}


int shader_init(void)
{
  int i;
  GLuint h;
  char fullpath[512];

  // load and compile shaders
  for(i=0;i<SHADER_COUNT;i++) {
    strncpy(fullpath, respath, 511);
    strncat(fullpath, "shaders/", 511);
    strncat(fullpath, shaderfile[i], 511);

    h=loadshader(fullpath, GL_FRAGMENT_SHADER);
    shader_program[i]=glCreateProgram();
    glAttachShader(shader_program[i], h);
    glLinkProgram(shader_program[i]);
    programerrors(shader_program[i]);
  }
  
  return 1;
}
