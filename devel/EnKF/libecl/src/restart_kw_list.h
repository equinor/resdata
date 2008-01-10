#ifndef __RESTART_KW_LIST_H__
#define __RESTART_KW_LIST_H__


typedef struct restart_kw_list_struct restart_kw_list_type;

restart_kw_list_type * restart_kw_list_alloc();
void                   restart_kw_list_free(restart_kw_list_type * );
void                   restart_kw_list_add(restart_kw_list_type * , const char *);
void                   restart_kw_list_reset(restart_kw_list_type *);
const char *           restart_kw_list_get_next(restart_kw_list_type *);

#endif
