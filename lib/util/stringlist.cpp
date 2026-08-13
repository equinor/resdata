#include <cstring>
#include <cstdlib>

#include <ert/util/ert_api_config.hpp>

#include <ert/util/util.hpp>
#include <ert/util/stringlist.hpp>
#include <ert/util/vector.hpp>

/**
   This file implements a very thin wrapper around a list (vector) of
   strings, and the total number of strings. It is mostly to avoid
   sending both argc and argv.

   Most of the functionality is implemented through vector.cpp and
   stateless functions in util.cpp
*/

struct stringlist_struct {
    vector_type *strings;
};

/**
   This function appends a copy of s into the stringlist.
*/
void stringlist_append_copy(stringlist_type *stringlist, const char *s) {
    if (s)
        vector_append_buffer(stringlist->strings, s, strlen(s) + 1);
    else
        vector_append_ref(stringlist->strings, NULL);
}

void stringlist_iset_copy(stringlist_type *stringlist, int index,
                          const char *s) {
    vector_iset_buffer(stringlist->strings, index, s, strlen(s) + 1);
}

static stringlist_type *stringlist_alloc_empty(bool alloc_vector) {
    stringlist_type *stringlist =
        (stringlist_type *)util_malloc(sizeof *stringlist);

    if (alloc_vector)
        stringlist->strings = vector_alloc_new();
    else
        stringlist->strings = NULL;

    return stringlist;
}

stringlist_type *stringlist_alloc_new() { return stringlist_alloc_empty(true); }

/**
    Frees all the memory contained by the stringlist.
*/
void stringlist_clear(stringlist_type *stringlist) {
    vector_clear(stringlist->strings);
}

void stringlist_free(stringlist_type *stringlist) {
    stringlist_clear(stringlist);
    vector_free(stringlist->strings);
    free(stringlist);
}

const char *stringlist_iget(const stringlist_type *stringlist, int index) {
    return (const char *)vector_iget(stringlist->strings, index);
}

const char *stringlist_front(const stringlist_type *stringlist) {
    return (const char *)vector_iget(stringlist->strings, 0);
}

const char *stringlist_back(const stringlist_type *stringlist) {
    return (const char *)vector_iget(stringlist->strings,
                                     vector_get_size(stringlist->strings) - 1);
}

int stringlist_get_size(const stringlist_type *stringlist) {
    return vector_get_size(stringlist->strings);
}
