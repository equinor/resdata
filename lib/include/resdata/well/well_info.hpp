#ifndef ERT_WELL_INFO_H
#define ERT_WELL_INFO_H

#include <resdata/rd_file.hpp>
#include <resdata/rd_grid.hpp>

#include <resdata/well/well_ts.hpp>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct well_info_struct well_info_type;

well_info_type *well_info_alloc(const rd_grid_type *grid);
void well_info_load_rstfile(well_info_type *well_info, const char *filename,
                            bool load_segment_information);
void well_info_load_rst_resfile(well_info_type *well_info,
                                rd_file_type *rst_file,
                                bool load_segment_information);
void well_info_free(well_info_type *well_info);

well_ts_type *well_info_get_ts(const well_info_type *well_info,
                               const char *well_name);
int well_info_get_num_wells(const well_info_type *well_info);
const char *well_info_iget_well_name(const well_info_type *well_info,
                                     int well_index);
bool well_info_has_well(well_info_type *well_info, const char *well_name);
#ifdef __cplusplus
}
#endif

#endif
