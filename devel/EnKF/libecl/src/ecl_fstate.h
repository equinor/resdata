#ifndef __ECL_FSTATE_H__
#define __ECL_FSTATE_H__

#include <stdbool.h>
#include <ecl_kw.h>
#include <ecl_block.h>

typedef struct ecl_fstate_struct ecl_fstate_type;
#define ECL_FORMATTED 	  0
#define ECL_BINARY    	  1   
#define ECL_FMT_AUTO      2
#define ECL_FMT_AUTO_BIT8 3
#define ECL_FMT_AUTO_NAME 4

bool              ecl_fstate_fmt_file(const char *filename);
char           ** ecl_fstate_alloc_filelist(const char *, const char *, const char *, int *);
void              ecl_fstate_set_multiple_files(ecl_fstate_type *, const char * , const char *);
void              ecl_fstate_set_unified_file(ecl_fstate_type *, const char *);
void              ecl_fstate_set_unified(ecl_fstate_type *ecl_fstate_type , bool unified);
void              ecl_fstate_add_block(ecl_fstate_type * , const ecl_block_type *);
bool              ecl_fstate_set_fmt_mode(ecl_fstate_type * , int);
ecl_fstate_type * ecl_fstate_alloc_empty(int , bool , bool );
ecl_fstate_type * ecl_fstate_load_unified (const char *, int , bool );
ecl_fstate_type * ecl_fstate_load_multiple(int , const char ** , int  , bool );
void              ecl_fstate_free(ecl_fstate_type *);
void            * ecl_fstate_kw_get_data_ref(const ecl_fstate_type * , int , const char *);
bool              ecl_fstate_kw_iget(const ecl_fstate_type * , int , const char *, int , void *);
bool              ecl_fstate_kw_get_memcpy_data(const ecl_fstate_type * , int , const char * , void *);
ecl_block_type  * ecl_fstate_get_block(const ecl_fstate_type * , int );
ecl_kw_type     * ecl_fstate_get_kw(const ecl_fstate_type * , int , const char *);
int               ecl_fstate_kw_get_size(const ecl_fstate_type * , int , const char *);
bool              ecl_fstate_kw_exists(const ecl_fstate_type *, int  , const char *);
int               ecl_fstate_get_Nstep(const ecl_fstate_type *);
void              ecl_fstate_save(const ecl_fstate_type *);
#endif
