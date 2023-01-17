/*
 This header does not define datatypes; just a couple of functions. It should
 be included from the ecl_kw.h header, so applications do not need to include this
 header explicitly.
*/

#ifndef ERT_ECL_KW_GRDECL_H
#define ERT_ECL_KW_GRDECL_H
#ifdef __cplusplus
extern "C" {
#endif

bool ecl_kw_grdecl_fseek_kw(const char *, bool, FILE *);

ecl_kw_type *ecl_kw_fscanf_alloc_grdecl_dynamic__(FILE *stream, const char *kw,
                                                  bool strict, ecl_data_type);
ecl_kw_type *ecl_kw_fscanf_alloc_grdecl_dynamic(FILE *stream, const char *kw,
                                                ecl_data_type);

ecl_kw_type *ecl_kw_fscanf_alloc_grdecl_data__(FILE *stream, bool strict,
                                               int size,
                                               ecl_data_type data_type);
ecl_kw_type *ecl_kw_fscanf_alloc_grdecl_data(FILE *stream, int size,
                                             ecl_data_type data_type);

ecl_kw_type *ecl_kw_fscanf_alloc_grdecl__(FILE *stream, const char *kw,
                                          bool strict, int size,
                                          ecl_data_type data_type);
ecl_kw_type *ecl_kw_fscanf_alloc_grdecl(FILE *stream, const char *kw, int size,
                                        ecl_data_type data_type);

ecl_kw_type *ecl_kw_fscanf_alloc_current_grdecl__(FILE *stream, bool strict,
                                                  ecl_data_type data_type);
ecl_kw_type *ecl_kw_fscanf_alloc_current_grdecl(FILE *stream,
                                                ecl_data_type data_type);

bool ecl_kw_grdecl_fseek_next_kw(FILE *stream);
char *ecl_kw_grdecl_alloc_next_header(FILE *stream);

void ecl_kw_fprintf_grdecl(const ecl_kw_type *ecl_kw, FILE *stream);
void ecl_kw_fprintf_grdecl__(const ecl_kw_type *ecl_kw,
                             const char *special_header, FILE *stream);

#ifdef __cplusplus
}
#endif
#endif
