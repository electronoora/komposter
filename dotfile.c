/*
 * Komposter (c) 2010-2011 Jani Halme (Firehawk/TDA)
 *
 * This code is licensed under the GNU General Public
 * License version 2. See file 'doc/LICENSE'.
 *
 * Handling the configuration file ~/.komposter
 *
 * $Rev$
 * $Date$
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "dotfile.h"

typedef struct {
  char *key;
  char *value;
} confkey;

confkey configdata[256];


int dotfile_load() {
  FILE *f;
  char *home, dotfile[256], line[256];
  char key[256], value[256], *t, *s;
  int i;
  
  home=getenv("HOME");

  i=0;
  strncpy(dotfile, home, 255);
  strncat(dotfile, "/.komposter", 255);
  //  printf("opening config from %s\n", dotfile);
  
  f=fopen(dotfile, "r");
  if (f) {
    while (fgets(line, 255, f)) {
      if (line[0]!='#' && strchr(line, '=')) {
        t=strchr(line, '='); t[0]='\0'; t++;
        s=strchr(t, '\n'); s[0]='\0';
        // printf("%s = %s\n", line, t);
/*
        configdata[i].key=malloc(strlen(line)+1);
        configdata[i].value=malloc(strlen(t)+1);
        strncpy(configdata[i].key, line, strlen(line));
        strncpy(configdata[i].value, t, strlen(t));
*/
        configdata[i].key=malloc(512); //strlen(line)+1);
        configdata[i].value=malloc(512); //strlen(t)+1);
        strncpy(configdata[i].key, line, 511); //strlen(line));
        strncpy(configdata[i].value, t, 511); //strlen(t));

        i++;
      }
      if (i>255) break;
    }
    fclose(f);
  } else {
    // no dotfile yet, create with paths defaulting to $HOME
    dotfile_setvalue("synthFileDir", home);
    dotfile_setvalue("songFileDir", home);
    dotfile_save();
  }
}


int dotfile_save() {
  FILE *f;
  int i;
  char *home, line[256], dotfile[256];
  
  home=getenv("HOME");

  i=0;
  strncpy(dotfile, home, 255);
  strncat(dotfile, "/.komposter", 255);
  f=fopen(dotfile, "w");
  if (f) {
    fprintf(f, "# This file is created automatically - only edit it if you know what you're doing!\n");
    for(i=0;i<255;i++) {
      if (configdata[i].key) {
        strncpy(line, configdata[i].key, 255);
        strncat(line, "=", 255);
        strncat(line, configdata[i].value, 255);
        strncat(line, "\n", 255);
        fputs(line, f);
      }
    }
    fclose(f);
  }
}



char *dotfile_getvalue(char *key) {
  int i;
  
  for (i=0;i<255;i++) {
    if (configdata[i].key) {
      if (!strcmp(configdata[i].key, key)) return configdata[i].value;
    }
  }
  return NULL;
}


int dotfile_setvalue(char *key, char *value) {
  int i;
  
  for (i=0;i<255;i++) {
    if (configdata[i].key) {
      if (!strcmp(configdata[i].key, key)) {
        if (configdata[i].value) free(configdata[i].value);
        configdata[i].value=malloc(strlen(value));
        strcpy(configdata[i].value, value);
        // printf("replaced key %s with value %s in index %d\n",key,value,i);
        return i;
      }
    }
  }
  for (i=0;i<255;i++) {
    if (!configdata[i].key) {
      configdata[i].key=malloc(strlen(key));
      configdata[i].value=malloc(strlen(value));
      strcpy(configdata[i].key, key);
      strcpy(configdata[i].value, value);
      // printf("stored new key %s with value %s to index %d\n",key,value,i);
      return i;
    }
  }
  
  return -1;  
}
    