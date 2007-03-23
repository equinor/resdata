#ifndef __RMS_UTIL_H__
#define __RMS_UTIL_H__
#include <stdio.h>
#include <stdlib.h>

void   rms_set_fortran_data(void *, const void * , int , int , int  , int);
void   rms_read_fortran_data(const void *, void * , int , int , int , int);
void   rms_fskip_string(FILE *);
int    rms_fread_strlen(FILE *);
bool   rms_fread_string(FILE * , char * ,  int);

#endif
