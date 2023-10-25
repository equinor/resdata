#ifndef ERT_WELL_RSEG_LOADER_H
#define ERT_WELL_RSEG_LOADER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <resdata/rd_file_view.hpp>

typedef struct well_rseg_loader_struct well_rseg_loader_type;

well_rseg_loader_type *well_rseg_loader_alloc(rd_file_view_type *rst_view);
void well_rseg_loader_free(well_rseg_loader_type *well_rseg_loader);

int well_rseg_loader_element_count(
    const well_rseg_loader_type *well_rseg_loader);
double *
well_rseg_loader_load_values(const well_rseg_loader_type *well_rseg_loader,
                             int rseg_offset);

#ifdef __cplusplus
}
#endif

#endif
