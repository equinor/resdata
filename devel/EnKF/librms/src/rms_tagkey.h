#ifndef __RMS_TAGKEY_H__
#define __RMS_TAGKEY_H__

#include <hash.h>
#include <stdio.h>


typedef struct rms_tagkey_struct rms_tagkey_type;


void              rms_free_tagkey(rms_tagkey_type *);
rms_tagkey_type * rms_alloc_empty_tagkey();
const char      * rms_tagkey_get_name(const rms_tagkey_type *);


bool              rms_tagkey_char_eq(const rms_tagkey_type *, const char *);
void              rms_free_tagkey_(void *);
const      void * rms_tagkey_copyc_(const void *);
void              rms_tagkey_load(rms_tagkey_type *, FILE *, hash_type *);
void            * rms_tagkey_get_data_ref(const rms_tagkey_type *);

#endif
