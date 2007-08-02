#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <ecl_util.h>


int ecl_util_filename_report_nr(const char *filename) {
  const char char_X = 'X';
  const char char_F = 'F';
  const char char_S = 'S';
  const char char_A = 'A';
  
  int report_nr;
  char *ext = strrchr(filename , '.');
  if (ext == NULL) {
    fprintf(stderr,"%s: can not determine timestep from filename:%s - aborting \n",__func__ , filename);
    abort();
  }
  
  if (ext[1] == char_X || ext[1] == char_F || ext[1] == char_S || ext[1] == char_A) 
    report_nr = atoi(&ext[2]);
  else {
    fprintf(stderr,"%s: Filename:%s not recognized - valid extensions: Annnn / Xnnnn / Fnnnn / Snnnn - aborting \n",__func__ , filename);
    abort();
  } 
  
  return report_nr;
}
