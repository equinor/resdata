#ifndef __MULTZ_H__
#define __MULTZ_H__

typedef struct multz_mem_struct multz_mem_type;

multz_mem_type * multz_mem_alloc(const mem_config_type * , const multz_config_type * );
void             multz_mem_free(multz_mem_type *);
char           * multz_mem_alloc_ensname(const multz_mem_type *);
char           * multz_mem_alloc_eclname(const multz_mem_type *);
#endif
