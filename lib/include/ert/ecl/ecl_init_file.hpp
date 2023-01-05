#ifndef ERT_ECL_INIT_FILE_H
#define ERT_ECL_INIT_FILE_H

#include <time.h>

#include <ert/ecl/fortio.h>
#include <ert/ecl/ecl_kw.hpp>
#include <ert/ecl/ecl_grid.hpp>
#include <ert/ecl/ecl_util.hpp>

#ifdef __cplusplus
extern "C" {
#endif

void ecl_init_file_fwrite_header(fortio_type *fortio, const ecl_grid_type *grid,
                                 const ecl_kw_type *poro,
                                 ert_ecl_unit_enum unit_system, int phases,
                                 time_t start_date);

#ifdef __cplusplus
}
#endif
#endif
