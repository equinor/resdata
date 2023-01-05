#ifndef ERT_GRID_DIMS_H
#define ERT_GRID_DIMS_H
#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int nx, ny, nz, nactive;
} grid_dims_type;

void grid_dims_init(grid_dims_type *dims, int nx, int ny, int nz, int nactive);
grid_dims_type *grid_dims_alloc(int nx, int ny, int nz, int nactive);
void grid_dims_free(grid_dims_type *dims);
void grid_dims_free__(void *arg);

#ifdef __cplusplus
}
#endif
#endif
