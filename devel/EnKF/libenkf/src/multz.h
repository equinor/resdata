#ifndef __MULTZ_H__
#define __MULTZ_H__

typedef struct multz_struct multz_type;

multz_type * multz_alloc(const mem_config_type * , const multz_config_type * );
void             multz_free(multz_type *);
char           * multz_alloc_ensname(const multz_type *);
char           * multz_alloc_eclname(const multz_type *);
void             multz_ecl_write(const multz_type * );
#endif
