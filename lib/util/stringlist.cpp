#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <ert/util/ert_api_config.hpp>
#ifdef ERT_HAVE_OPENDIR
#include <sys/types.h>
#include <dirent.h>
#endif

#ifdef ERT_HAVE_GLOB
#include <glob.h>
#else
#include <Windows.h>
#endif

#include <ert/util/util.hpp>
#include <ert/util/stringlist.hpp>
#include <ert/util/vector.hpp>

#define STRINGLIST_TYPE_ID 671855

/**
   This file implements a very thin wrapper around a list (vector) of
   strings, and the total number of strings. It is mostly to avoid
   sending both argc and argv.

   Most of the functionality is implemented through vector.c and
   stateless functions in util.c
*/

#ifdef __cplusplus
extern "C" {
#endif

struct stringlist_struct {
    UTIL_TYPE_ID_DECLARATION;
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

void stringlist_iset_owned_ref(stringlist_type *stringlist, int index,
                               const char *s) {
    vector_iset_owned_ref(stringlist->strings, index, s, free);
}

static stringlist_type *stringlist_alloc_empty(bool alloc_vector) {
    stringlist_type *stringlist =
        (stringlist_type *)util_malloc(sizeof *stringlist);
    UTIL_TYPE_ID_INIT(stringlist, STRINGLIST_TYPE_ID);

    if (alloc_vector)
        stringlist->strings = vector_alloc_new();
    else
        stringlist->strings = NULL;

    return stringlist;
}

stringlist_type *stringlist_alloc_new() { return stringlist_alloc_empty(true); }

stringlist_type *stringlist_alloc_argv_copy(const char **argv, int argc) {
    int iarg;
    stringlist_type *stringlist = stringlist_alloc_empty(true);
    for (iarg = 0; iarg < argc; iarg++)
        stringlist_append_copy(stringlist, argv[iarg]);

    return stringlist;
}

void stringlist_append_stringlist_copy(stringlist_type *stringlist,
                                       const stringlist_type *src) {
    int i;
    for (i = 0; i < stringlist_get_size(src); i++)
        stringlist_append_copy(stringlist, stringlist_iget(src, i));
}

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

static UTIL_SAFE_CAST_FUNCTION(stringlist, STRINGLIST_TYPE_ID);
UTIL_IS_INSTANCE_FUNCTION(stringlist, STRINGLIST_TYPE_ID)

char *stringlist_pop(stringlist_type *stringlist) {
    return (char *)vector_pop_back(stringlist->strings);
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

double stringlist_iget_as_double(const stringlist_type *stringlist, int index,
                                 bool *valid) {
    const char *string_value = stringlist_iget(stringlist, index);
    double value = -1.0;

    if (valid != NULL)
        *valid = false;

    if (util_sscanf_double(string_value, &value))
        if (valid != NULL)
            *valid = true;

    return value;
}

const char *stringlist_get_last(const stringlist_type *stringlist) {
    return (const char *)vector_get_last(stringlist->strings);
}

char *stringlist_iget_copy(const stringlist_type *stringlist, int index) {
    return util_alloc_string_copy(stringlist_iget(stringlist, index));
}

int stringlist_get_size(const stringlist_type *stringlist) {
    return vector_get_size(stringlist->strings);
}

/**
    Scans the stringlist (linear scan) to see if it contains (at
    least) one occurence of 's'. Will never return true if the input
    string @s equals NULL, altough the stringlist itself can contain
    NULL elements.
*/

bool stringlist_contains(const stringlist_type *stringlist, const char *s) {
    int size = stringlist_get_size(stringlist);
    int index = 0;
    bool contains = false;

    while ((index < size) && (!contains)) {
        const char *istring = stringlist_iget(stringlist, index);
        if (istring != NULL)
            if (strcmp(istring, s) == 0)
                contains = true;
        index++;
    }

    return contains;
}

/**
  Finds the indicies of the entries matching 's'.
*/
int_vector_type *stringlist_find(const stringlist_type *stringlist,
                                 const char *s) {
    int_vector_type *indicies = int_vector_alloc(0, -1);
    int size = stringlist_get_size(stringlist);
    int index = 0;

    while (index < size) {
        const char *istring = stringlist_iget(stringlist, index);
        if (istring != NULL)
            if (strcmp(istring, s) == 0)
                int_vector_append(indicies, index);
        index++;
    }
    return indicies;
}

/**
  Find the index of the first index matching 's'.
  Returns -1 if 's' cannot be found.
*/
int stringlist_find_first(const stringlist_type *stringlist, const char *s) {
    bool found = false;
    int size = stringlist_get_size(stringlist);
    int index = 0;

    while (index < size && !found) {
        const char *istring = stringlist_iget(stringlist, index);
        if (istring != NULL)
            if (strcmp(istring, s) == 0) {
                found = true;
                break;
            }
        index++;
    }

    if (found)
        return index;
    else
        return -1;
}

bool stringlist_equal(const stringlist_type *s1, const stringlist_type *s2) {
    int size1 = stringlist_get_size(s1);
    int size2 = stringlist_get_size(s2);
    if (size1 == size2) {
        bool equal = true;
        int i;
        for (i = 0; i < size1; i++) {
            if (strcmp(stringlist_iget(s1, i), stringlist_iget(s2, i)) != 0) {
                equal = false;
                break;
            }
        }
        return equal;
    } else
        return false;
}

static int strcmp__(const void *__s1, const void *__s2) {
    const char *s1 = (const char *)__s1;
    const char *s2 = (const char *)__s2;
    return strcmp(s1, s2);
}

/**
   Will sort the stringlist inplace. The prototype of the comparison
   function is

     int (cmp) (const void * , const void *);

   i.e. ths strings are implemented as (void *). If string_cmp == NULL
   the sort function will use the ordinary strcmp() function for
   comparison.
*/

void stringlist_sort(stringlist_type *s, string_cmp_ftype *string_cmp) {
    if (string_cmp == NULL)
        vector_sort(s->strings, strcmp__);
    else
        vector_sort(s->strings, string_cmp);
}

void stringlist_python_sort(stringlist_type *s, int cmp_flag) {
    if (cmp_flag == 0)
        stringlist_sort(s, NULL);
    else if (cmp_flag == 1)
        stringlist_sort(s, (string_cmp_ftype *)util_strcmp_int);
    else if (cmp_flag == 2)
        stringlist_sort(s, (string_cmp_ftype *)util_strcmp_float);
    else
        util_abort("%s: unrecognized cmp_flag:%d \n", __func__, cmp_flag);
}

void stringlist_reverse(stringlist_type *s) {
    vector_inplace_reverse(s->strings);
}

/*
  This function uses the stdlib function glob() to select file/path
  names matching a pattern. The stringlist is cleared when the
  function starts.
*/

#ifdef ERT_HAVE_GLOB
int stringlist_select_matching(stringlist_type *names, const char *pattern) {
    int match_count = 0;
    stringlist_clear(names);

    {
        size_t i;
        glob_t *pglob = (glob_t *)util_malloc(sizeof *pglob);
        int glob_flags = 0;
        glob(pattern, glob_flags, NULL, pglob);
        match_count = pglob->gl_pathc;
        for (i = 0; i < pglob->gl_pathc; i++)
            stringlist_append_copy(names, pglob->gl_pathv[i]);
        globfree(
            pglob); /* Only frees the _internal_ data structures of the pglob object. */
        free(pglob);
    }
    return match_count;
}
#endif

int stringlist_select_matching_files(stringlist_type *names, const char *path,
                                     const char *file_pattern) {
#ifdef ERT_HAVE_GLOB
    char *pattern = util_alloc_filename(path, file_pattern, NULL);
    int match_count = stringlist_select_matching(names, pattern);
    free(pattern);
    return match_count;
#else
    {
        WIN32_FIND_DATA file_data;
        HANDLE file_handle;
        char *pattern = util_alloc_filename(path, file_pattern, NULL);

        stringlist_clear(names);
        file_handle = FindFirstFile(pattern, &file_data);
        if (file_handle != INVALID_HANDLE_VALUE) {
            do {
                char *full_path =
                    util_alloc_filename(path, file_data.cFileName, NULL);
                stringlist_append_copy(names, full_path);
                free(full_path);
            } while (FindNextFile(file_handle, &file_data) != 0);
        }
        FindClose(file_handle);
        free(pattern);

        return stringlist_get_size(names);
    }
#endif
}

int stringlist_select_files(stringlist_type *names, const char *path,
                            file_pred_ftype *predicate, const void *pred_arg) {
    stringlist_clear(names);
    char *path_arg = path ? util_alloc_string_copy(path) : util_alloc_cwd();

#ifdef ERT_HAVE_OPENDIR
    DIR *dir = opendir(path_arg);
    if (!dir) {
        free(path_arg);
        return 0;
    }

    while (true) {
        struct dirent *entry = readdir(dir);
        if (!entry)
            break;

        if (util_string_equal(entry->d_name, "."))
            continue;

        if (util_string_equal(entry->d_name, ".."))
            continue;

        if (predicate && !predicate(entry->d_name, pred_arg))
            continue;

        {
            char *fname = util_alloc_filename(path, entry->d_name, NULL);
            stringlist_append_copy(names, fname);
            free(fname);
        }
    }

    closedir(dir);

#else

    WIN32_FIND_DATA file_data;
    HANDLE file_handle;
    char *pattern = util_alloc_filename(path_arg, "*", NULL);

    file_handle = FindFirstFile(pattern, &file_data);
    if (file_handle != INVALID_HANDLE_VALUE) {
        do {
            if (util_string_equal(file_data.cFileName, "."))
                continue;

            if (util_string_equal(file_data.cFileName, ".."))
                continue;

            if (predicate && !predicate(file_data.cFileName, pred_arg))
                continue;
            {
                char *tmp_fname =
                    util_alloc_filename(path, file_data.cFileName, NULL);
                stringlist_append_copy(names, tmp_fname);
                free(tmp_fname);
            }
        } while (FindNextFile(file_handle, &file_data) != 0);
        FindClose(file_handle);
    }
    free(pattern);

#endif

    free(path_arg);
    return stringlist_get_size(names);
}

#ifdef __cplusplus
}
#endif
