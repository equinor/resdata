#ifndef __ECL_FSTATE_H__
#define __ECL_FSTATE_H__

#include <stdbool.h>
#include <ecl_kw.h>

typedef struct ecl_fstate_struct ecl_fstate_type;
#define ECL_FORMATTED 	  0
#define ECL_BINARY    	  1   
#define ECL_FMT_AUTO      2
#define ECL_FMT_AUTO_BIT8 3
#define ECL_FMT_AUTO_NAME 4


ecl_fstate_type * ecl_fstate_load_unified (const char *, int , bool );
ecl_fstate_type * ecl_fstate_load_multiple(int , const char ** , int  , bool );
void              ecl_fstate_free(ecl_fstate_type *);
bool              ecl_fstate_kw_iget(const ecl_fstate_type * , int , const char *, int , void *);
bool              ecl_fstate_kw_get_memcpy_data(const ecl_fstate_type * , int , const char * , void *);
ecl_kw_type     * ecl_fstate_get_kw(const ecl_fstate_type * , int , const char *);
int               ecl_fstate_kw_get_size(const ecl_fstate_type * , int , const char *);
bool              ecl_fstate_kw_exists(const ecl_fstate_type *, int  , const char *);
int               ecl_fstate_get_blocksize(const ecl_fstate_type *);
#endif
