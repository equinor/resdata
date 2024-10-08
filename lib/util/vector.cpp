#include <stdlib.h>
#include <string.h>

#include <ert/util/util.hpp>
#include <ert/util/node_data.hpp>
#include <ert/util/type_macros.hpp>
#include <ert/util/vector.hpp>

#define VECTOR_TYPE_ID 551087
#define VECTOR_DEFAULT_SIZE 10

struct vector_struct {
    UTIL_TYPE_ID_DECLARATION;
    int alloc_size; /* The number of elements allocated in the data vector - in general > size. */
    int size; /* THe number of elements the user has added to the vector. */
    node_data_type *
        *data; /* node_data instances - which again contain user data. */
};

/* Small datastructure PURELY used for sorting the vector. */

typedef struct {
    vector_cmp_ftype *user_cmp;
    node_data_type *data;
    int index;
} vector_sort_node_type;

UTIL_SAFE_CAST_FUNCTION(vector, VECTOR_TYPE_ID)
UTIL_IS_INSTANCE_FUNCTION(vector, VECTOR_TYPE_ID)

static void vector_resize__(vector_type *vector, int new_alloc_size) {
    int i;
    if (new_alloc_size < vector->alloc_size) {
        /* The vector is shrinking. */
        for (i = new_alloc_size; i < vector->alloc_size; i++)
            node_data_free(vector->data[i]);
    }

    vector->data = (node_data_type **)util_realloc(
        vector->data, new_alloc_size * sizeof *vector->data);
    for (i = vector->alloc_size; i < new_alloc_size; i++)
        vector->data[i] = NULL; /* Initialising new nodes to NULL */

    vector->alloc_size = new_alloc_size;
}

vector_type *vector_alloc_new() {
    vector_type *vector = (vector_type *)util_malloc(sizeof *vector);
    UTIL_TYPE_ID_INIT(vector, VECTOR_TYPE_ID);
    vector->size = 0;
    vector->alloc_size = 0;
    vector->data = NULL;
    vector_resize__(vector, VECTOR_DEFAULT_SIZE);
    return vector;
}

static void vector_grow_NULL(vector_type *vector, int new_size) {
    int i;
    for (i = vector->size; i < new_size; i++)
        vector_append_ref(vector, NULL);
}

static int vector_append_node(vector_type *vector, node_data_type *node);

/**
    If the index is beyond the length of the vector the hole in the
    vector will be filled with NULL nodes.
*/

static void vector_iset__(vector_type *vector, int index,
                          node_data_type *node) {
    if (index > vector->size)
        vector_grow_NULL(vector, index);

    if (index == vector->size)
        vector_append_node(vector, node);
    else {
        if (vector->data[index] != NULL)
            node_data_free(vector->data[index]);

        vector->data[index] = node;
    }
}

/**
   This is the low level function opposite to the vector_idel()
   function. A new value (node) is inserted at index, and the rest of
   the vector is shifted to the right.
*/

static void vector_insert__(vector_type *vector, int index,
                            node_data_type *node) {
    if (vector->size == vector->alloc_size)
        vector_resize__(vector, 2 * (vector->alloc_size + 1));
    {
        int bytes_to_move = (vector->size - index) * sizeof *vector->data;
        memmove(&vector->data[index + 1], &vector->data[index], bytes_to_move);
        vector->data[index] =
            NULL; /* Otherwise the destructor might try to pick up on it in the vector_iset__() call below */
    }
    vector->size++;
    vector_iset__(vector, index, node);
}

/**
   This is the low-level append node function which actually "does
   it", the node has been allocated in one of the front-end
   functions. The return value is the index of the node (which can
   subsequently be used in a vector_iget() call)).
*/
static int vector_append_node(vector_type *vector, node_data_type *node) {
    if (vector->size == vector->alloc_size)
        vector_resize__(vector, 2 * (vector->alloc_size + 1));

    vector->size++;
    vector_iset__(vector, vector->size - 1, node);
    return vector->size - 1;
}

/*
  This is like the vector_append_node() function, but the node is
  pushed in at the front.
*/

static void vector_push_node(vector_type *vector, node_data_type *node) {
    if (vector->size == vector->alloc_size)
        vector_resize__(vector, 2 * (vector->alloc_size + 1));
    {
        int bytes = vector->size * sizeof *vector->data;
        if (bytes > 0) {
            memmove(&vector->data[1], vector->data, bytes);
            vector->data[0] =
                NULL; /* Otherwise the destructor might try to pick up on it in the vector_iset__() call below */
        }
    }
    vector->size++;
    vector_iset__(vector, 0, node);
}

/**
   Will append NULL pointers until the vectors length is equal to
   @min_size.
*/

static void vector_assert_size(vector_type *vector, int min_size) {
    while (vector->size < min_size)
        vector_append_ref(vector, NULL);
}

/**
   Append a user-pointer which comes without either copy constructor
   or destructor, this implies that the calling scope has FULL
   responsabilty for the storage of the data added to the vector.
*/

int vector_append_ref(vector_type *vector, const void *data) {
    node_data_type *node = node_data_alloc_ptr(data, NULL, NULL);
    return vector_append_node(vector, node);
}

void vector_iset_ref(vector_type *vector, int index, const void *data) {
    node_data_type *node = node_data_alloc_ptr(data, NULL, NULL);
    vector_iset__(vector, index, node);
}

/**
   Append a user-pointer which the vector instance takes ownership
   of. This means that when the vector is destroyed it calls the
   destructor on the data which has been supplied. The calling scope
   should basically let this object be - the vector has taken control.
*/

int vector_append_owned_ref(vector_type *vector, const void *data,
                            free_ftype *del) {
    node_data_type *node = node_data_alloc_ptr(data, NULL, del);
    return vector_append_node(vector, node);
}

void vector_iset_owned_ref(vector_type *vector, int index, const void *data,
                           free_ftype *del) {
    node_data_type *node = node_data_alloc_ptr(data, NULL, del);
    vector_iset__(vector, index, node);
}

/**
   A buffer is unstructured storage (i.e. a void *) which is destroyed
   with free, and copied with malloc + memcpy. The vector takes a copy
   of the buffer which is inserted (and freed on vector destruction).
*/

void vector_append_buffer(vector_type *vector, const void *buffer,
                          int buffer_size) {
    node_data_type *node = node_data_alloc_buffer(buffer, buffer_size);
    vector_append_node(vector, node);
}

void vector_iset_buffer(vector_type *vector, int index, const void *buffer,
                        int buffer_size) {
    node_data_type *node = node_data_alloc_buffer(buffer, buffer_size);
    vector_iset__(vector, index, node);
}

const void *vector_iget_const(const vector_type *vector, int index) {
    if ((index >= 0) && (index < vector->size)) {
        const node_data_type *node = vector->data[index];
        return node_data_get_ptr(node);
    } else {
        util_abort("%s: Invalid index:%d  Valid range: [0,%d> \n", __func__,
                   index, vector->size);
        return NULL;
    }
}

void *vector_iget(const vector_type *vector, int index) {
    if ((index >= 0) && (index < vector->size)) {
        const node_data_type *node = vector->data[index];
        return node_data_get_ptr(node);
    } else {
        util_abort("%s: Invalid index:%d  Valid range: [0,%d> \n", __func__,
                   index, vector->size);
        return NULL;
    }
}

void *vector_safe_iget(const vector_type *vector, int index) {
    if ((index >= 0) && (index < vector->size)) {
        const node_data_type *node = vector->data[index];
        return node_data_get_ptr(node);
    } else
        return NULL;
}

/*
   Removes element nr index from the vector, if a destructor is
   associated with element 'index' it is called, and the memory
   freed. Afterwards all elements at positions (index +1) and onwards
   are shifted one element to the left.
*/

void vector_idel(vector_type *vector, int index) {
    if ((index >= 0) && (index < vector->size)) {
        node_data_type *node = vector->data[index];
        node_data_free(node); /* Discard the element */
        {
            int bytes_to_move =
                (vector->size - 1 - index) * sizeof *vector->data;
            memmove(&vector->data[index], &vector->data[index + 1],
                    bytes_to_move);
            vector->data[vector->size - 1] =
                NULL; /* Clear the last element  - which is no longer valid. */
            vector->size--;
        }
    } else
        util_abort("%s: Invalid index:%d  Valid range: [0,%d> \n", __func__,
                   index, vector->size);
}

/**
   This function will remove the last elements of the vector, so that
   the new size becomes @new_size. If the @new_size is greater than
   the current length of the vector the function will fail HARD.
*/

void vector_shrink(vector_type *vector, int new_size) {
    if (new_size > vector->size)
        util_abort(
            "%s: Tried to \'shrink\' vector to %d elements - current size:%d\n",
            __func__, new_size, vector->size);
    {
        int index;
        for (index = (vector->size - 1); index >= new_size; index--)
            vector_idel(vector, index);
    }
}

/**
    Will abort if the vector is empty.
*/
void *vector_get_last(const vector_type *vector) {
    if (vector->size == 0)
        util_abort("%s: asking to get the last element in an empty vector - "
                   "impossible ... \n",
                   __func__);
    {
        const node_data_type *node = vector->data[vector->size - 1];
        return node_data_get_ptr(node);
    }
}

/**
   This function removes the last element from the vector and returns
   it to the calling scope. Irrespective of whether the element _was_
   inserted with a destructor: when calling vector_pop() the calling
   scope takes responsability for freeing data; i.e. vector_pop will
   NEVER call a destructor.
*/

void *vector_pop_back(vector_type *vector) {
    if (vector->size == 0)
        util_abort("%s: asking to get the last element in an empty vector - "
                   "impossible ... \n",
                   __func__);
    {
        node_data_type *node = vector->data[vector->size - 1];
        void *data = node_data_get_ptr(node);

        node_data_free_container(node); /* Free the container holding data. */
        vector->data[vector->size - 1] = NULL;
        vector->size--; /* Shrink the vector */
        return data;
    }
}

int vector_get_size(const vector_type *vector) { return vector->size; }

/**
   This vector frees all the storage of the vector, including all the
   nodes which have been installed with a destructor.
*/

void vector_clear(vector_type *vector) {
    int i;
    for (i = 0; i < vector->size; i++) {
        node_data_free(
            vector->data[i]); /* User specific destructors are called here. */
        vector->data[i] =
            NULL; /* This is essential to protect against unwaranted calls to destructors when data is reused. */
    }
    vector->size = 0;
}

void vector_free(vector_type *vector) {
    vector_clear(vector);
    free(vector->data);
    free(vector);
}

void vector_free__(void *arg) { vector_free(vector_safe_cast(arg)); }

static int vector_cmp(const void *s1, const void *s2) {
    const vector_sort_node_type *node1 = (const vector_sort_node_type *)s1;
    const vector_sort_node_type *node2 = (const vector_sort_node_type *)s2;

    return node1->user_cmp(node_data_get_ptr(node1->data),
                           node_data_get_ptr(node2->data));
}

/**
   This function will sort the vector content in place. The sorting is
   based on a user-supplied cmp function which should return -1,0,1
   when comparing two elements. The prototype of this function is

       int (* user_cmp) (const void *, const void *)

   i.e. the same as for qsort. The vector implementation considers
   (fully) untyped data, it is therefore the users responsability to
   ensure that the comparison makes sense. For example:


     double * p = util_malloc(10 * sizeof * p );
     vector_append_buffer(vector , "This is a string ..." , strlen());
     vector_append_buffer(vector , p , 10 * sizeof * p);

   Here we have inserted one (char *) and one (double *). When these
   elements arrive in the sort function they will just be (void *),
   and the comparison will be quite meaningless(??).
*/

static vector_sort_node_type *vector_alloc_sort_data(const vector_type *vector,
                                                     vector_cmp_ftype *cmp) {
    vector_sort_node_type *sort_data =
        (vector_sort_node_type *)util_calloc(vector->size, sizeof *sort_data);
    int i;

    /* Fill up the temporary storage used for sorting */
    for (i = 0; i < vector->size; i++) {
        sort_data[i].data = vector->data[i];
        sort_data[i].user_cmp = cmp;
        sort_data[i].index = i;
    }

    /* Sort the temporary vector */
    if (sort_data) {
        qsort(sort_data, vector->size, sizeof *sort_data, vector_cmp);
    }

    return sort_data;
}

void vector_sort(vector_type *vector, vector_cmp_ftype *cmp) {
    vector_sort_node_type *sort_data = vector_alloc_sort_data(vector, cmp);
    int i;
    /* Recover the sorted vector */
    for (i = 0; i < vector->size; i++)
        vector->data[i] = sort_data[i].data;

    free(sort_data);
}

int_vector_type *vector_alloc_sort_perm(const vector_type *vector,
                                        vector_cmp_ftype *cmp) {
    vector_sort_node_type *sort_data = vector_alloc_sort_data(vector, cmp);
    int_vector_type *sort_perm = int_vector_alloc(0, 0);
    int i;
    int_vector_resize(sort_perm, vector->size, 0);
    for (i = 0; i < vector->size; i++)
        int_vector_iset(sort_perm, i, sort_data[i].index);

    free(sort_data);
    return sort_perm;
}

void vector_permute(vector_type *vector, const int_vector_type *perm_vector) {
    node_data_type **new_data =
        (node_data_type **)util_calloc(vector->size, sizeof *new_data);
    for (int index = 0; index < vector->size; index++) {
        int perm_index = int_vector_iget(perm_vector, index);
        new_data[index] = vector->data[perm_index];
    }
    free(vector->data);
    vector->data = new_data;
}

void vector_inplace_reverse(vector_type *vector) {
    if (vector->size > 0) {
        node_data_type **new_data =
            (node_data_type **)util_calloc(vector->size, sizeof *new_data);
        int index;
        for (index = 0; index < vector->size; index++) {
            int rev_index = vector->size - 1 - index;
            new_data[index] = vector->data[rev_index];
        }
        free(vector->data);
        vector->data = new_data;
    }
}

/*
  If deep_copy == true - all elements in the vector MUST have
  constructor, otherwise the node_data_alloc_copy() function will fail.
*/

vector_type *vector_alloc_copy(const vector_type *src, bool deep_copy) {
    vector_type *copy = vector_alloc_new();
    int i;
    for (i = 0; i < src->size; i++) {
        node_data_type *node_copy =
            node_data_alloc_copy(src->data[i], deep_copy);
        vector_append_node(copy, node_copy);
    }
    return copy;
}
