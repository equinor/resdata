#pragma once

#include <cstdio>

#include <resdata/rd_kw.hpp>

bool rd_kw_grdecl_fseek_kw(const char *, bool, FILE *);

rd_kw_type *rd_kw_fscanf_alloc_grdecl_dynamic(FILE *stream, const char *kw,
                                              bool strict, rd_data_type);

rd_kw_type *rd_kw_fscanf_alloc_grdecl(FILE *stream, const char *kw, int size,
                                      rd_data_type data_type);

void rd_kw_fprintf_grdecl(const rd_kw_type *rd_kw, FILE *stream);
void rd_kw_fprintf_grdecl__(const rd_kw_type *rd_kw, const char *special_header,
                            FILE *stream);
