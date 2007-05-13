#ifndef __RMS_TAGKEY_H__
#define __RMS_TAGKEY_H__

#include <hash.h>
#include <stdio.h>
#include <stdbool.h>
#include <rms_type.h>

typedef struct rms_tagkey_struct rms_tagkey_type;


void              rms_free_tagkey(rms_tagkey_type *);
rms_tagkey_type * rms_tagkey_alloc_empty(bool);
rms_tagkey_type * rms_tagkey_alloc_complete(const char * , int , rms_type_enum , const void * );
const char      * rms_tagkey_get_name(const rms_tagkey_type *);


bool              rms_tagkey_char_eq(const rms_tagkey_type *, const char *);
void              rms_free_tagkey_(void *);
const      void * rms_tagkey_copyc_(const void *);
void              rms_tagkey_load(rms_tagkey_type *, bool , FILE *, hash_type *);
void            * rms_tagkey_get_data_ref(const rms_tagkey_type *);
void              rms_tagkey_fwrite(const rms_tagkey_type * , FILE *);


rms_tagkey_type * rms_tagkey_alloc_byteswap();
rms_tagkey_type * rms_tagkey_alloc_creationDate();
rms_tagkey_type * rms_tagkey_alloc_filetype(const char * );

#endif
