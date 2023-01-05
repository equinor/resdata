#include <stdlib.h>

#include <ert/util/util.h>

#include <ert/ecl/grid_dims.hpp>

void grid_dims_init(grid_dims_type *dims, int nx, int ny, int nz, int nactive) {
    dims->nx = nx;
    dims->ny = ny;
    dims->nz = nz;
    dims->nactive = nactive;
}

grid_dims_type *grid_dims_alloc(int nx, int ny, int nz, int nactive) {
    grid_dims_type *dims = (grid_dims_type *)util_malloc(sizeof *dims);
    grid_dims_init(dims, nx, ny, nz, nactive);
    return dims;
}

void grid_dims_free(grid_dims_type *dims) { free(dims); }

void grid_dims_free__(void *arg) {
    grid_dims_type *grid_dims = (grid_dims_type *)arg;
    grid_dims_free(grid_dims);
}
