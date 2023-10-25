#ifndef ERT_RD_RST_FILE_H
#define ERT_RD_RST_FILE_H

#include <resdata/rd_rsthead.hpp>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct rd_rst_file_struct rd_rst_file_type;

rd_rst_file_type *rd_rst_file_open_read(const char *filename);
rd_rst_file_type *rd_rst_file_open_write(const char *filename);
rd_rst_file_type *rd_rst_file_open_append(const char *filename);
rd_rst_file_type *rd_rst_file_open_write_seek(const char *filename,
                                              int report_step);
void rd_rst_file_close(rd_rst_file_type *rst_file);

void rd_rst_file_start_solution(rd_rst_file_type *rst_file);
void rd_rst_file_end_solution(rd_rst_file_type *rst_file);
void rd_rst_file_fwrite_header(rd_rst_file_type *rst_file, int seqnum,
                               rd_rsthead_type *rsthead_data);
void rd_rst_file_add_kw(rd_rst_file_type *rst_file, const rd_kw_type *rd_kw);
offset_type rd_rst_file_ftell(const rd_rst_file_type *rst_file);

#ifdef __cplusplus
}
#endif

#endif
