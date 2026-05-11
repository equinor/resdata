#pragma once

#include <cstdio>
#include <optional>
#include <string>

#include <resdata/rd_kw.hpp>

bool rd_kw_grdecl_fseek_kw(const char *, bool, FILE *);

rd_kw_type *rd_kw_fscanf_alloc_grdecl(FILE *stream, const char *kw,
                                      rd_data_type data_type, int size = 0,
                                      bool strict = true);

void rd_kw_fprintf_grdecl(const rd_kw_type *rd_kw, FILE *stream,
                          const char *special_header = nullptr);
