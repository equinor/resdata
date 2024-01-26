#ifndef ERT_STRINGLIST_H
#define ERT_STRINGLIST_H

#include <stdbool.h>
#include <stdio.h>

#include <ert/util/ert_api_config.hpp>
#include <ert/util/type_macros.hpp>
#include <ert/util/int_vector.hpp>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct stringlist_struct stringlist_type;
typedef int(string_cmp_ftype)(const void *, const void *);
typedef bool(file_pred_ftype)(const char *, const void *);

int stringlist_select_files(stringlist_type *names, const char *path,
                            file_pred_ftype *predicate, const void *pred_arg);

const char *stringlist_get_last(const stringlist_type *stringlist);
char *stringlist_pop(stringlist_type *stringlist);

stringlist_type *stringlist_alloc_new(void);
void stringlist_free(stringlist_type *);
void stringlist_clear(stringlist_type *);

void stringlist_append_copy(stringlist_type *, const char *);

const char *stringlist_iget(const stringlist_type *, int);
double stringlist_iget_as_double(const stringlist_type *stringlist, int index,
                                 bool *valid);
char *stringlist_iget_copy(const stringlist_type *stringlist, int);
const char *stringlist_front(const stringlist_type *stringlist);
const char *stringlist_back(const stringlist_type *stringlist);

void stringlist_iset_copy(stringlist_type *, int index, const char *);
void stringlist_iset_owned_ref(stringlist_type *, int index, const char *);

int stringlist_get_size(const stringlist_type *);

stringlist_type *stringlist_alloc_argv_copy(const char **, int);

bool stringlist_equal(const stringlist_type *, const stringlist_type *);
bool stringlist_contains(const stringlist_type *, const char *);
int_vector_type *stringlist_find(const stringlist_type *, const char *);
int stringlist_find_first(const stringlist_type *, const char *);
void stringlist_sort(stringlist_type *, string_cmp_ftype *string_cmp);
void stringlist_reverse(stringlist_type *s);
void stringlist_python_sort(stringlist_type *s, int cmp_flag);

#ifdef ERT_HAVE_GLOB
int stringlist_select_matching(stringlist_type *names, const char *pattern);
#endif
int stringlist_select_matching_files(stringlist_type *names, const char *path,
                                     const char *file_pattern);
UTIL_IS_INSTANCE_HEADER(stringlist);

#ifdef __cplusplus
}
#endif
#endif
