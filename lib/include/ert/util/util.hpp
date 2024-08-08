#ifndef ERT_UTIL_H
#define ERT_UTIL_H

/*
   Copyright (C) 2011  Equinor ASA, Norway.

   The file 'util.hpp' is part of ERT - Ensemble based Reservoir Tool.

   ERT is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   ERT is distributed in the hope that it will be useful, but WITHOUT ANY
   WARRANTY; without even the implied warranty of MERCHANTABILITY or
   FITNESS FOR A PARTICULAR PURPOSE.

   See the GNU General Public License at <http://www.gnu.org/licenses/gpl.html>
   for more details.
*/

#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>
#include <sys/types.h>
#include <time.h>

#include <ert/util/ert_api_config.hpp>

#ifdef ERT_HAVE_GETUID
#include <sys/stat.h>
#endif

#ifdef ERT_WINDOWS
#define UTIL_PATH_SEP_STRING                                                   \
    "\\" /* A \0 terminated separator used when we want a (char *) instance.                   */
#define UTIL_PATH_SEP_CHAR                                                     \
    '\\' /* A simple character used when we want an actual char instance (i.e. not a pointer). */
#else
#define UTIL_PATH_SEP_STRING                                                   \
    "/" /* A \0 terminated separator used when we want a (char *) instance.                   */
#define UTIL_PATH_SEP_CHAR                                                     \
    '/' /* A simple character used when we want an actual char instance (i.e. not a pointer). */
#endif

#define UTIL_WINDOWS_PATH_SEP_CHAR '\\'
#define UTIL_POSIX_PATH_SEP_CHAR '/'

#define UTIL_NEWLINE_STRING "          \n"
#define UTIL_DEFAULT_MKDIR_MODE                                                \
    0777 /* Directories are by default created with mode a+rwx - and then comes the umask ... */

#ifdef __cplusplus
extern "C" {
#endif

/*
  These ifdefs are an attempt to support large files (> 2GB)
  transparently on both Windows and Linux. See source file
  libert_util/src/util_lfs.c for more details.

  During the configure step CMAKE should check the size of (void *)
  and set the ERT_WINDOWS_LFS variable to true if a 64 bit platform is
  detected.
*/

#ifdef ERT_WINDOWS_LFS
typedef struct _stat64 stat_type;
typedef __int64 offset_type;
#else
typedef struct stat stat_type;
#ifdef HAVE_FSEEKO
typedef off_t offset_type;
#else
typedef long offset_type;
#endif
#endif

typedef void(walk_file_callback_ftype)(
    const char *, /* The current directory */
    const char *, /* The current file / directory */
    void *);      /* Arbitrary argument */

typedef bool(walk_dir_callback_ftype)(
    const char *, /* The current directory */
    const char *, /* The current file / directory */
    int,          /* The current depth in the file hiearcrcy. */
    void *);      /* Arbitrary argument */

char *util_get_timezone(void);
time_t util_make_datetime_utc(int, int, int, int, int, int);
time_t util_make_date_utc(int, int, int);
time_t util_make_pure_date_utc(time_t t);
void util_inplace_forward_seconds_utc(time_t *t, double seconds);
void util_inplace_forward_days_utc(time_t *t, double days);

double util_difftime(time_t, time_t, int *, int *, int *, int *);
double util_difftime_days(time_t, time_t);
double util_difftime_seconds(time_t start_time, time_t end_time);

char *util_alloc_sprintf_va(const char *fmt, va_list ap);
char *util_alloc_sprintf(const char *, ...);
bool util_sscanf_isodate(const char *, time_t *);
bool util_entry_exists(const char *entry);
bool util_file_exists(const char *);
bool util_is_abs_path(const char *);
char *util_alloc_abs_path(const char *path);
char *util_alloc_rel_path(const char *__root_path, const char *path);
bool util_fmt_bit8(const char *);
bool util_mkdir_p(const char *);
void util_make_path(const char *);
double util_file_difftime(const char *, const char *);
size_t util_file_size(const char *);
size_t util_fd_size(int fd);
void util_clear_directory(const char *path, bool strict_uid, bool unlink_root);
void util_strupr(char *);
bool util_string_equal(const char *s1, const char *s2);
char *util_alloc_strupr_copy(const char *);
bool util_copy_file(const char *, const char *);
bool util_copy_file__(const char *src_file, const char *target_file,
                      size_t buffer_size, void *buffer, bool abort_on_error);
char *util_alloc_cwd(void);
bool util_is_cwd(const char *path);
char *util_alloc_normal_path(const char *input_path);
char *util_alloc_realpath(const char *);
char *util_alloc_realpath__(const char *input_path);
bool util_string_has_wildcard(const char *s);
bool util_ftruncate(FILE *stream, long size);

int util_roundf(float x);
int util_round(double x);

offset_type util_ftell(FILE *stream);
int util_fseek(FILE *stream, offset_type offset, int whence);
void util_rewind(FILE *stream);
int util_stat(const char *filename, stat_type *stat_info);
int util_fstat(int fileno, stat_type *stat_info);

#ifdef ERT_HAVE_OPENDIR
void util_copy_directory_content(const char *src_path, const char *target_path);
void util_copy_directory(const char *, const char *);
#endif

int util_forward_line(FILE *, bool *);
void util_rewind_line(FILE *);

FILE *util_mkdir_fopen(const char *filename, const char *mode);
FILE *util_fopen(const char *, const char *);
FILE *util_fopen__(const char *filename, const char *mode);
bool util_fopen_test(const char *, const char *);
char *util_split_alloc_filename(const char *input_path);
void util_alloc_file_components(const char *, char **, char **, char **);
char *util_alloc_tmp_file(const char *, const char *, bool);
char *util_fscanf_alloc_line(FILE *, bool *);
bool util_sscanf_bool(const char *, bool *);
int util_strcmp_int(const char *s1, const char *s2);
int util_strcmp_float(const char *s1, const char *s2);
bool util_sscanf_int(const char *, int *);
bool util_sscanf_double(const char *, double *);
char *util_alloc_filename(const char *, const char *, const char *);
char *util_alloc_strip_copy(const char *);
char *util_strcat_realloc(char *, const char *);
char *util_alloc_string_copy(const char *);
void util_path_split(const char *line, int *_tokens, char ***_token_list);
char *util_alloc_parent_path(const char *path);
void util_binary_split_string(const char *, const char *, bool, char **,
                              char **);
int util_string_replace_inplace(char **, const char *, const char *);
char *util_realloc_string_copy(char *, const char *);
void util_free_stringlist(char **, int);
char *util_alloc_substring_copy(const char *, int offset, int N);
bool util_is_directory(const char *);
bool util_is_file(const char *);
void util_set_datetime_values_utc(time_t, int *, int *, int *, int *, int *,
                                  int *);
void util_set_date_values_utc(time_t, int *, int *, int *);
bool util_is_first_day_in_month_utc(time_t t);
unsigned int util_clock_seed(void);
void util_fread_dev_random(int, char *);
void util_fread_dev_urandom(int, char *);
void util_abort_test_set_intercept_function(const char *);
void util_exit(const char *fmt, ...);
void util_install_signals(void);
void util_update_signals(void);

void *util_realloc(void *, size_t);
void *util_malloc(size_t);
void *util_calloc(size_t elements, size_t element_size);
void *util_realloc_copy(void *org_ptr, const void *src, size_t byte_size);
void *util_alloc_copy(const void *, size_t);
void util_double_to_float(float *, const double *, int);
void util_float_to_double(double *, const float *, int);
char *util_fread_alloc_file_content(const char *, int *);
void util_fwrite_string(const char *, FILE *);
char *util_fread_alloc_string(FILE *);
void util_endian_flip_vector(void *data, int element_size, int elements);

void util_double_vector_max_min(int, const double *, double *, double *);
void util_update_double_max_min(double, double *, double *);
void util_update_float_max_min(float, float *, float *);
void util_update_int_max_min(int, int *, int *);
int util_int_max(int, int);
double util_double_max(double, double);
int util_int_min(int, int);
size_t util_size_t_min(size_t a, size_t b);
size_t util_size_t_max(size_t a, size_t b);
double util_double_min(double, double);
void util_fskip_lines(FILE *, int);
bool util_same_file(const char *, const char *);
void util_fread(void *, size_t, size_t, FILE *, const char *);
void util_fwrite(const void *, size_t, size_t, FILE *, const char *);
int util_fread_int(FILE *);
void util_fwrite_offset(offset_type, FILE *);
void util_fwrite_size_t(size_t, FILE *);
void util_fwrite_int(int, FILE *);
void util_fwrite_long(long, FILE *);
void util_fwrite_double(double, FILE *);
void util_fwrite_int_vector(const int *, int, FILE *, const char *);
int util_get_current_linenr(FILE *stream);
bool util_fseek_string(FILE *stream, const char *string, bool skip_string,
                       bool case_sensitive);
bool util_files_equal(const char *file1, const char *file2);
double util_kahan_sum(const double *data, size_t N);
bool util_double_approx_equal(double d1, double d2);
bool util_double_approx_equal__(double d1, double d2, double rel_eps,
                                double abs_eps);
bool util_float_approx_equal__(float d1, float d2, float rel_eps,
                               float abs_eps);
int util_fnmatch(const char *pattern, const char *string);
void util_time_utc(time_t *t, struct tm *ts);

bool util_is_link(const char *); // Will always return false on windows
int util_chdir(const char *path);

#ifdef ERT_HAVE_UNISTD
#include <unistd.h>
bool util_access(const char *entry, mode_t mode);
#else
bool util_access(const char *entry, int mode);
#define F_OK 0
#define R_OK 4
#define W_OK 2
#define X_OK 1
#endif

#define UTIL_FWRITE_SCALAR(s, stream)                                          \
    {                                                                          \
        if (fwrite(&s, sizeof s, 1, stream) != 1)                              \
            util_abort("%s: write failed: %s\n", __func__, strerror(errno));   \
    }

#define UTIL_FREAD_SCALAR(s, stream)                                           \
    {                                                                          \
        int fread_return = fread(&s, sizeof s, 1, stream);                     \
        if (fread_return == 0) {                                               \
            if (errno == 0)                                                    \
                util_abort("%s: read failed - premature EOF\n", __func__);     \
                                                                               \
            util_abort("%s: read failed: %s\n", __func__, strerror(errno));    \
        }                                                                      \
    }

#define UTIL_FWRITE_VECTOR(s, n, stream)                                       \
    {                                                                          \
        if (fwrite(s, sizeof s, (n), stream) != (n))                           \
            util_abort("%s: write failed: %s \n", __func__, strerror(errno));  \
    }
#define UTIL_FREAD_VECTOR(s, n, stream)                                        \
    {                                                                          \
        if (fread(s, sizeof s, (n), stream) != (n))                            \
            util_abort("%s: read failed: %s \n", __func__, strerror(errno));   \
    }

#define CONTAINS_HEADER(TYPE)                                                  \
    int util_sorted_contains_##TYPE(const TYPE *data, int size, TYPE value)
CONTAINS_HEADER(int);
CONTAINS_HEADER(time_t);
CONTAINS_HEADER(size_t);
#undef CONTAINS_HEADER

typedef void util_abort_handler_t(const char *filename, int lineno,
                                  const char *function, const char *message,
                                  const char *backtrace);
void util_set_abort_handler(util_abort_handler_t *);

#ifdef _MSC_VER
#define util_abort(fmt, ...)                                                   \
    util_abort__(__FILE__, __func__, __LINE__, fmt, __VA_ARGS__)
#elif __GNUC__
/* Also clang defines the __GNUC__ symbol */
#define util_abort(fmt, ...)                                                   \
    util_abort__(__FILE__, __func__, __LINE__, fmt, ##__VA_ARGS__)
#endif

/* Conditional section below here */

void util_abort__(const char *file, const char *function, int line,
                  const char *fmt, ...);
void util_abort_signal(int);

#ifdef ERT_HAVE_SYMLINK
void util_make_slink(const char *, const char *);
char *util_alloc_link_target(const char *link);
#ifdef ERT_HAVE_READLINKAT
char *util_alloc_atlink_target(const char *path, const char *link);
#endif
#endif

#include "util_unlink.hpp"

#ifdef __cplusplus
}
#endif
#endif
