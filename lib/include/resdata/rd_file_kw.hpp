#pragma once
#include <cstdio>

#include <memory>
#include <vector>
#include <string>

#include <ert/util/util.hpp>

#include <resdata/rd_kw.hpp>
#include <resdata/FortIO.hpp>
#include "resdata/rd_type.hpp"

typedef struct rd_file_kw_struct rd_file_kw_type;

struct rd_file_kw_struct {
    offset_type file_offset;
    rd_data_type data_type;
    int kw_size;
    int ref_count = 0;
    std::string header;
    rd_kw_ptr kw{nullptr, &rd_kw_free};

    rd_file_kw_struct(offset_type file_offset, rd_data_type data_type,
                      int kw_size, std::string header)
        : file_offset(file_offset), data_type(data_type), kw_size(kw_size),
          header(header) {};
};

typedef struct inv_map_struct inv_map_type;
using rd_file_kw_ptr = std::unique_ptr<rd_file_kw_type>;

inv_map_type *inv_map_alloc();
rd_file_kw_type *inv_map_get_file_kw(inv_map_type *inv_map,
                                     const rd_kw_type *rd_kw);
void inv_map_free(inv_map_type *map);
bool rd_file_kw_equal(const rd_file_kw_type *kw1, const rd_file_kw_type *kw2);
rd_file_kw_type *rd_file_kw_alloc(const rd_kw_type *rd_kw, offset_type offset);
rd_file_kw_type *rd_file_kw_alloc0(const char *header, rd_data_type data_type,
                                   int size, offset_type offset);
void rd_file_kw_free(rd_file_kw_type *file_kw);
rd_kw_type *rd_file_kw_get_kw_ptr(rd_file_kw_type *file_kw);
const char *rd_file_kw_get_header(const rd_file_kw_type *file_kw);
int rd_file_kw_get_size(const rd_file_kw_type *file_kw);
rd_data_type rd_file_kw_get_data_type(const rd_file_kw_type *);
offset_type rd_file_kw_get_offset(const rd_file_kw_type *file_kw);
bool rd_file_kw_fskip_data(const rd_file_kw_type *file_kw, ERT::FortIO &fortio);

void rd_file_kw_fwrite(const rd_file_kw_type *file_kw, FILE *stream);
std::vector<rd_file_kw_ptr> rd_file_kw_fread(FILE *stream, int num);

void rd_file_kw_start_transaction(const rd_file_kw_type *file_kw,
                                  int *ref_count);
void rd_file_kw_end_transaction(rd_file_kw_type *file_kw, int ref_count);

rd_kw_type *rd_file_kw_get_kw(rd_file_kw_type *file_kw, ERT::FortIO &fortio,
                              inv_map_type *inv_map);
void rd_file_kw_inplace_fwrite(rd_file_kw_type *file_kw, ERT::FortIO &fortio);
