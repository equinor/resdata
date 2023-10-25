#include <stdlib.h>
#include <stdbool.h>

#include <ert/util/util.h>
#include <ert/util/vector.hpp>

#include <resdata/rd_grid_dims.hpp>
#include <resdata/rd_util.hpp>
#include <resdata/fortio.h>
#include <resdata/rd_endian_flip.hpp>
#include <resdata/rd_kw_magic.hpp>
#include <resdata/rd_kw.hpp>

struct rd_grid_dims_struct {
    vector_type *dims_list;
};

static void rd_grid_dims_read_EGRID(rd_grid_dims_type *grid_dims,
                                    fortio_type *grid_fortio,
                                    fortio_type *data_fortio) {
    while (rd_kw_fseek_kw(GRIDHEAD_KW, false, false, grid_fortio)) {
        grid_dims_type *dims;
        {
            rd_kw_type *gridhead_kw = rd_kw_fread_alloc(grid_fortio);

            int nx = rd_kw_iget_int(gridhead_kw, GRIDHEAD_NX_INDEX);
            int ny = rd_kw_iget_int(gridhead_kw, GRIDHEAD_NY_INDEX);
            int nz = rd_kw_iget_int(gridhead_kw, GRIDHEAD_NZ_INDEX);

            dims = grid_dims_alloc(nx, ny, nz, 0);
            rd_kw_free(gridhead_kw);
        }

        if (data_fortio) {
            if (rd_kw_fseek_kw(INTEHEAD_KW, false, false, data_fortio)) {
                rd_kw_type *intehead_kw = rd_kw_fread_alloc(data_fortio);
                dims->nactive =
                    rd_kw_iget_int(intehead_kw, INTEHEAD_NACTIVE_INDEX);
                rd_kw_free(intehead_kw);
            }
        }

        vector_append_owned_ref(grid_dims->dims_list, dims, grid_dims_free__);
    }
}

static void rd_grid_dims_read_GRID(rd_grid_dims_type *grid_dims,
                                   fortio_type *grid_fortio,
                                   fortio_type *data_fortio) {
    while (rd_kw_fseek_kw(DIMENS_KW, false, false, grid_fortio)) {
        grid_dims_type *dims;
        {
            rd_kw_type *dimens_kw = rd_kw_fread_alloc(grid_fortio);

            int nx = rd_kw_iget_int(dimens_kw, DIMENS_NX_INDEX);
            int ny = rd_kw_iget_int(dimens_kw, DIMENS_NY_INDEX);
            int nz = rd_kw_iget_int(dimens_kw, DIMENS_NZ_INDEX);

            dims = grid_dims_alloc(nx, ny, nz, 0);
            rd_kw_free(dimens_kw);
        }

        if (data_fortio) {
            if (rd_kw_fseek_kw(INTEHEAD_KW, false, false, data_fortio)) {
                rd_kw_type *intehead_kw = rd_kw_fread_alloc(data_fortio);
                dims->nactive =
                    rd_kw_iget_int(intehead_kw, INTEHEAD_NACTIVE_INDEX);
                rd_kw_free(intehead_kw);
            }
        }

        vector_append_owned_ref(grid_dims->dims_list, dims, grid_dims_free__);
    }
}

rd_grid_dims_type *rd_grid_dims_alloc(const char *grid_file,
                                      const char *data_file) {
    rd_grid_dims_type *grid_dims = NULL;
    bool grid_fmt_file;
    rd_file_enum grid_file_type =
        rd_get_file_type(grid_file, &grid_fmt_file, NULL);

    if ((grid_file_type == RD_GRID_FILE) || (grid_file_type == RD_EGRID_FILE)) {
        fortio_type *grid_fortio =
            fortio_open_reader(grid_file, grid_fmt_file, RD_ENDIAN_FLIP);
        if (grid_fortio) {
            grid_dims = (rd_grid_dims_type *)util_malloc(sizeof *grid_dims);
            grid_dims->dims_list = vector_alloc_new();

            {
                fortio_type *data_fortio = NULL;
                bool data_fmt_file;

                if (data_file) {
                    rd_get_file_type(data_file, &data_fmt_file, NULL);
                    data_fortio = fortio_open_reader(data_file, data_fmt_file,
                                                     RD_ENDIAN_FLIP);
                }

                if (grid_file_type == RD_EGRID_FILE)
                    rd_grid_dims_read_EGRID(grid_dims, grid_fortio,
                                            data_fortio);
                else
                    rd_grid_dims_read_GRID(grid_dims, grid_fortio, data_fortio);

                if (data_fortio)
                    fortio_fclose(data_fortio);
            }
            fortio_fclose(grid_fortio);
        }
    }

    return grid_dims;
}

void rd_grid_dims_free(rd_grid_dims_type *grid_dims) {
    vector_free(grid_dims->dims_list);
    free(grid_dims);
}

int rd_grid_dims_get_num_grids(const rd_grid_dims_type *grid_dims) {
    return vector_get_size(grid_dims->dims_list);
}

const grid_dims_type *rd_grid_dims_iget_dims(const rd_grid_dims_type *grid_dims,
                                             int grid_nr) {
    return (const grid_dims_type *)vector_iget_const(grid_dims->dims_list,
                                                     grid_nr);
}
