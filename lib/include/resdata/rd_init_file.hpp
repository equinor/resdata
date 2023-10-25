#ifndef ERT_RD_INIT_FILE_H
#define ERT_RD_INIT_FILE_H

#include <time.h>

#include <resdata/fortio.h>
#include <resdata/rd_kw.hpp>
#include <resdata/rd_grid.hpp>
#include <resdata/rd_util.hpp>

#ifdef __cplusplus
extern "C" {
#endif

void rd_init_file_fwrite_header(fortio_type *fortio, const rd_grid_type *grid,
                                const rd_kw_type *poro,
                                ert_rd_unit_enum unit_system, int phases,
                                time_t start_date);

#ifdef __cplusplus
}
#endif
#endif
