/*
 This header does not define datatypes; just a couple of functions. It should
 be included from the rd_kw.h header, so applications do not need to include this
 header explicitly.
*/

#ifndef ERT_RD_KW_GRDECL_H
#define ERT_RD_KW_GRDECL_H
#ifdef __cplusplus
extern "C" {
#endif

bool rd_kw_grdecl_fseek_kw(const char *, bool, FILE *);

rd_kw_type *rd_kw_fscanf_alloc_grdecl_dynamic__(FILE *stream, const char *kw,
                                                bool strict, rd_data_type);
rd_kw_type *rd_kw_fscanf_alloc_grdecl_dynamic(FILE *stream, const char *kw,
                                              rd_data_type);

rd_kw_type *rd_kw_fscanf_alloc_grdecl_data__(FILE *stream, bool strict,
                                             int size, rd_data_type data_type);
rd_kw_type *rd_kw_fscanf_alloc_grdecl_data(FILE *stream, int size,
                                           rd_data_type data_type);

rd_kw_type *rd_kw_fscanf_alloc_grdecl__(FILE *stream, const char *kw,
                                        bool strict, int size,
                                        rd_data_type data_type);
rd_kw_type *rd_kw_fscanf_alloc_grdecl(FILE *stream, const char *kw, int size,
                                      rd_data_type data_type);

rd_kw_type *rd_kw_fscanf_alloc_current_grdecl__(FILE *stream, bool strict,
                                                rd_data_type data_type);
rd_kw_type *rd_kw_fscanf_alloc_current_grdecl(FILE *stream,
                                              rd_data_type data_type);

bool rd_kw_grdecl_fseek_next_kw(FILE *stream);
char *rd_kw_grdecl_alloc_next_header(FILE *stream);

void rd_kw_fprintf_grdecl(const rd_kw_type *rd_kw, FILE *stream);
void rd_kw_fprintf_grdecl__(const rd_kw_type *rd_kw, const char *special_header,
                            FILE *stream);

#ifdef __cplusplus
}
#endif
#endif
