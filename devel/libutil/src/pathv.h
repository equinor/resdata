#ifndef __PATHV_H__
#define __PATHV_H__

typedef struct pathv_struct pathv_type;

pathv_type * pathv_alloc(int , const char **);
void             pathv_iset(pathv_type * , int , const char *);
void             pathv_vset(pathv_type * , const char **);
void             pathv_free(pathv_type *);
const char     * pathv_get_ref(const pathv_type *);

#endif

