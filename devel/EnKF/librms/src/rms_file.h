#ifndef __RMS_FILE_H__
#define __RMS_FILE_H__

#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>



typedef struct rms_file_struct   rms_file_type;

void                 rms_load  (const char * , const char * , float *);
rms_file_type      * rms_open  (const char *, bool , bool );
void                 rms_close (rms_file_type *);



#endif
