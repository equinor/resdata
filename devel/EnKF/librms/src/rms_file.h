#ifndef __RMS_FILE_H__
#define __RMS_FILE_H__
#include <stdbool.h>
#include <stdlib.h>


typedef struct rms_file_struct   rms_file_type;

void                 rms_load(const char * , const char * , float *);
rms_file_type * rms_open      (const char *, bool , bool );
void                 rms_close     (rms_file_type *);

rms_tag_type  * rms_fread_alloc_tag(const rms_file_type *, bool *);




#endif
