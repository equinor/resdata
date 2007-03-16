#ifndef __RMS_TAG_H__
#define __RMS_TAG_H__
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <hash.h>



typedef struct rms_tag_struct rms_tag_type;

void           rms_tag_free(rms_tag_type *);
rms_tag_type * rms_tag_fread_alloc(FILE *, hash_type *, bool *);

#endif
