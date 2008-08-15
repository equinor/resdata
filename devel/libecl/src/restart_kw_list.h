#ifndef __RESTART_KW_LIST_H__
#define __RESTART_KW_LIST_H__
#include <stdio.h>

typedef struct restart_kw_list_struct restart_kw_list_type;

restart_kw_list_type * restart_kw_list_alloc();
void                   restart_kw_list_free(restart_kw_list_type * );
void                   restart_kw_list_add(restart_kw_list_type * , const char *);
void                   restart_kw_list_reset(restart_kw_list_type *);
const char *           restart_kw_list_get_next(restart_kw_list_type *);
const char *           restart_kw_list_get_first(restart_kw_list_type * );
void                   restart_kw_list_memcpy(restart_kw_list_type * , restart_kw_list_type * );
void 		       restart_kw_list_fwrite(const restart_kw_list_type *  , FILE * );
void 		       restart_kw_list_fread(restart_kw_list_type * , FILE * );
restart_kw_list_type * restart_kw_list_fread_alloc(FILE * );
bool                   restart_kw_list_modified(const restart_kw_list_type * );
bool                   restart_kw_list_empty(const restart_kw_list_type * );
void                   restart_kw_list_fprintf(const restart_kw_list_type * , FILE * );

#endif
