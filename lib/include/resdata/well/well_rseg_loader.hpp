#pragma once

#include <resdata/rd_file_view.hpp>

typedef struct well_rseg_loader_struct well_rseg_loader_type;

well_rseg_loader_type *well_rseg_loader_alloc(rd::FileView *rst_view);
void well_rseg_loader_free(well_rseg_loader_type *well_rseg_loader);

double *well_rseg_loader_load_values(well_rseg_loader_type *well_rseg_loader,
                                     size_t rseg_offset);
