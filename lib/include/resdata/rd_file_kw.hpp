#ifndef ERT_RD_FILE_KW_H
#define ERT_RD_FILE_KW_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

#include <ert/util/util.h>

#include <resdata/rd_kw.hpp>
#include <resdata/fortio.h>

typedef struct rd_file_kw_struct rd_file_kw_type;
typedef struct inv_map_struct inv_map_type;

inv_map_type *inv_map_alloc(void);
rd_file_kw_type *inv_map_get_file_kw(inv_map_type *inv_map,
                                     const rd_kw_type *rd_kw);
void inv_map_free(inv_map_type *map);
bool rd_file_kw_equal(const rd_file_kw_type *kw1, const rd_file_kw_type *kw2);
rd_file_kw_type *rd_file_kw_alloc(const rd_kw_type *rd_kw, offset_type offset);
rd_file_kw_type *rd_file_kw_alloc0(const char *header, rd_data_type data_type,
                                   int size, offset_type offset);
void rd_file_kw_free(rd_file_kw_type *file_kw);
void rd_file_kw_free__(void *arg);
rd_kw_type *rd_file_kw_get_kw(rd_file_kw_type *file_kw, fortio_type *fortio,
                              inv_map_type *inv_map);
rd_kw_type *rd_file_kw_get_kw_ptr(rd_file_kw_type *file_kw);
rd_file_kw_type *rd_file_kw_alloc_copy(const rd_file_kw_type *src);
const char *rd_file_kw_get_header(const rd_file_kw_type *file_kw);
int rd_file_kw_get_size(const rd_file_kw_type *file_kw);
rd_data_type rd_file_kw_get_data_type(const rd_file_kw_type *);
offset_type rd_file_kw_get_offset(const rd_file_kw_type *file_kw);
bool rd_file_kw_ptr_eq(const rd_file_kw_type *file_kw, const rd_kw_type *rd_kw);
void rd_file_kw_replace_kw(rd_file_kw_type *file_kw, fortio_type *target,
                           rd_kw_type *new_kw);
bool rd_file_kw_fskip_data(const rd_file_kw_type *file_kw, fortio_type *fortio);
void rd_file_kw_inplace_fwrite(rd_file_kw_type *file_kw, fortio_type *fortio);

void rd_file_kw_fwrite(const rd_file_kw_type *file_kw, FILE *stream);
rd_file_kw_type **rd_file_kw_fread_alloc_multiple(FILE *stream, int num);
rd_file_kw_type *rd_file_kw_fread_alloc(FILE *stream);

void rd_file_kw_start_transaction(const rd_file_kw_type *file_kw,
                                  int *ref_count);
void rd_file_kw_end_transaction(rd_file_kw_type *file_kw, int ref_count);

#ifdef __cplusplus
}
#endif

#endif
