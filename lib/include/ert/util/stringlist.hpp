#pragma once
#include <memory>

#include <ert/util/ert_api_config.hpp>

typedef struct stringlist_struct stringlist_type;
typedef int(string_cmp_ftype)(const void *, const void *);
typedef bool(file_pred_ftype)(const char *, const void *);

stringlist_type *stringlist_alloc_new(void);
void stringlist_free(stringlist_type *);
void stringlist_clear(stringlist_type *);

void stringlist_append_copy(stringlist_type *, const char *);

const char *stringlist_iget(const stringlist_type *, int);

void stringlist_iset_copy(stringlist_type *, int index, const char *);

int stringlist_get_size(const stringlist_type *);

using stringlist_ptr =
    std::unique_ptr<stringlist_type, decltype(&stringlist_free)>;

inline stringlist_ptr make_stringlist() {
    return {stringlist_alloc_new(), &stringlist_free};
}
