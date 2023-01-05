#ifndef PERM_VECTOR_H
#define PERM_VECTOR_H

#include <ert/util/type_macros.hpp>

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

typedef struct perm_vector_struct perm_vector_type;

perm_vector_type *perm_vector_alloc(int *perm_input, int size);
void perm_vector_free(perm_vector_type *perm_vector);
int perm_vector_get_size(const perm_vector_type *perm);
int perm_vector_iget(const perm_vector_type *perm, int index);

#ifdef __cplusplus
}
#endif //__cplusplus

#endif
