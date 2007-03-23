#ifndef __RMS_FILE_H__
#define __RMS_FILE_H__

#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <rms_tag.h>


typedef struct rms_file_struct   rms_file_type;

void                 rms_file_load   (rms_file_type *);
rms_file_type      * rms_open  	     (const char *, bool , bool );
void                 rms_close 	     (rms_file_type *);
rms_tag_type       * rms_file_get_tag(const rms_file_type *, const char *, const char *, const char *);


#endif
