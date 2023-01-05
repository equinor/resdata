#ifndef ERT_ECL_RST_FILE_H
#define ERT_ECL_RST_FILE_H

#include <ert/ecl/ecl_rsthead.hpp>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ecl_rst_file_struct ecl_rst_file_type;

ecl_rst_file_type *ecl_rst_file_open_read(const char *filename);
ecl_rst_file_type *ecl_rst_file_open_write(const char *filename);
ecl_rst_file_type *ecl_rst_file_open_append(const char *filename);
ecl_rst_file_type *ecl_rst_file_open_write_seek(const char *filename,
                                                int report_step);
void ecl_rst_file_close(ecl_rst_file_type *rst_file);

void ecl_rst_file_start_solution(ecl_rst_file_type *rst_file);
void ecl_rst_file_end_solution(ecl_rst_file_type *rst_file);
void ecl_rst_file_fwrite_header(ecl_rst_file_type *rst_file, int seqnum,
                                ecl_rsthead_type *rsthead_data);
void ecl_rst_file_add_kw(ecl_rst_file_type *rst_file,
                         const ecl_kw_type *ecl_kw);
offset_type ecl_rst_file_ftell(const ecl_rst_file_type *rst_file);

#ifdef __cplusplus
}
#endif

#endif
