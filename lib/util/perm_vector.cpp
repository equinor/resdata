#include <stdlib.h>

#include <ert/util/type_macros.hpp>
#include <ert/util/util.h>
#include <ert/util/perm_vector.hpp>

#define PERM_VECTOR_TYPE_ID 661433

struct perm_vector_struct {
    UTIL_TYPE_ID_DECLARATION;
    int size;
    int *perm;
};

/*
  This constructor will *take ownership* of the input int* array; and
  call free( ) on it when the perm_vector is destroyed.
*/

perm_vector_type *perm_vector_alloc(int *perm_input, int size) {
    perm_vector_type *perm = (perm_vector_type *)util_malloc(sizeof *perm);
    UTIL_TYPE_ID_INIT(perm, PERM_VECTOR_TYPE_ID);
    perm->size = size;
    perm->perm = perm_input;
    return perm;
}

void perm_vector_free(perm_vector_type *perm) {
    free(perm->perm);
    free(perm);
}

int perm_vector_get_size(const perm_vector_type *perm) { return perm->size; }

int perm_vector_iget(const perm_vector_type *perm, int index) {
    if (index < perm->size)
        return perm->perm[index];
    else {
        util_abort("%s: invalid index:%d \n", __func__, index);
        return -1;
    }
}
