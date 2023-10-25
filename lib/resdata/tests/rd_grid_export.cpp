#include <stdlib.h>
#include <stdbool.h>

#include <ert/util/test_util.hpp>
#include <ert/util/test_work_area.hpp>

#include <resdata/rd_grid.hpp>
#include <resdata/rd_kw.hpp>
#include <resdata/rd_file.hpp>

void export_actnum(const rd_grid_type *rd_grid, rd_file_type *rd_file) {
    rd_kw_type *actnum_kw = rd_file_iget_named_kw(rd_file, "ACTNUM", 0);
    int *actnum =
        (int *)util_malloc(rd_kw_get_size(actnum_kw) * sizeof *actnum);

    rd_grid_init_actnum_data(rd_grid, actnum);
    for (int i = 0; i < rd_kw_get_size(actnum_kw); i++)
        test_assert_int_equal(actnum[i], rd_kw_iget_int(actnum_kw, i));

    free(actnum);
}

void export_coord(const rd_grid_type *grid, rd_file_type *rd_file) {
    rd_kw_type *coord_kw = rd_file_iget_named_kw(rd_file, "COORD", 0);
    test_assert_int_equal(rd_kw_get_size(coord_kw),
                          rd_grid_get_coord_size(grid));
    {
        float *coord_float = (float *)util_malloc(rd_grid_get_coord_size(grid) *
                                                  sizeof *coord_float);
        double *coord_double = (double *)util_malloc(
            rd_grid_get_coord_size(grid) * sizeof *coord_double);

        rd_grid_init_coord_data(grid, coord_float);
        rd_grid_init_coord_data_double(grid, coord_double);

        for (int i = 0; i < rd_grid_get_coord_size(grid); i++)
            test_assert_double_equal(coord_double[i], coord_float[i]);

        free(coord_float);
        free(coord_double);
    }
}

void export_zcorn(const rd_grid_type *grid, rd_file_type *rd_file) {
    rd_kw_type *zcorn_kw = rd_file_iget_named_kw(rd_file, "ZCORN", 0);
    test_assert_int_equal(rd_kw_get_size(zcorn_kw),
                          rd_grid_get_zcorn_size(grid));
    {
        float *zcorn_float = (float *)util_malloc(rd_grid_get_zcorn_size(grid) *
                                                  sizeof *zcorn_float);
        double *zcorn_double = (double *)util_malloc(
            rd_grid_get_zcorn_size(grid) * sizeof *zcorn_double);

        rd_grid_init_zcorn_data(grid, zcorn_float);
        rd_grid_init_zcorn_data_double(grid, zcorn_double);

        for (int i = 0; i < rd_grid_get_zcorn_size(grid); i++) {
            test_assert_double_equal(zcorn_double[i], zcorn_float[i]);
            test_assert_float_equal(zcorn_float[i],
                                    rd_kw_iget_float(zcorn_kw, i));
        }

        free(zcorn_float);
        free(zcorn_double);
    }
}

void copy_processed(const rd_grid_type *src) {
    {
        rd_grid_type *copy = rd_grid_alloc_processed_copy(src, NULL, NULL);
        test_assert_true(rd_grid_compare(src, copy, true, true, false));
        rd_grid_free(copy);
    }

    {
        int *actnum =
            (int *)util_malloc(rd_grid_get_global_size(src) * sizeof *actnum);
        int index = 0;
        rd_grid_init_actnum_data(src, actnum);

        while (true) {
            if (actnum[index] == 1) {
                actnum[index] = 0;
                break;
            }
            index++;
        }

        {
            rd_grid_type *copy =
                rd_grid_alloc_processed_copy(src, NULL, actnum);
            test_assert_int_equal(1, rd_grid_get_active_size(src) -
                                         rd_grid_get_active_size(copy));
            rd_grid_free(copy);
        }
        free(actnum);
    }

    {
        double *zcorn_double = (double *)util_malloc(
            rd_grid_get_zcorn_size(src) * sizeof *zcorn_double);
        int i = 0;
        int j = 0;
        int k = 0;

        rd_grid_init_zcorn_data_double(src, zcorn_double);
        {
            rd_grid_type *copy =
                rd_grid_alloc_processed_copy(src, zcorn_double, NULL);
            test_assert_double_equal(rd_grid_get_cell_volume3(src, i, j, k),
                                     rd_grid_get_cell_volume3(copy, i, j, k));
            rd_grid_free(copy);
        }

        for (int c = 0; c < 4; c++) {
            double dz = zcorn_double[rd_grid_zcorn_index(src, i, j, k, c + 4)] -
                        zcorn_double[rd_grid_zcorn_index(src, i, j, k, c)];
            zcorn_double[rd_grid_zcorn_index(src, i, j, k, c + 4)] += dz;
        }
        {
            rd_grid_type *copy =
                rd_grid_alloc_processed_copy(src, zcorn_double, NULL);
            test_assert_double_equal(rd_grid_get_cell_volume3(src, i, j, k) * 2,
                                     rd_grid_get_cell_volume3(copy, i, j, k));
            rd_grid_free(copy);
        }

        free(zcorn_double);
    }
}

void export_mapaxes(const rd_grid_type *grid, rd_file_type *rd_file) {
    if (rd_file_has_kw(rd_file, "MAPAXES")) {
        rd_kw_type *mapaxes_kw = rd_file_iget_named_kw(rd_file, "MAPAXES", 0);
        double mapaxes[6];
        int i;

        test_assert_true(rd_grid_use_mapaxes(grid));
        rd_grid_init_mapaxes_data_double(grid, mapaxes);
        for (i = 0; i < 6; i++)
            test_assert_double_equal(rd_kw_iget_float(mapaxes_kw, i),
                                     mapaxes[i]);
    }
}

int main(int argc, char **argv) {
    rd::util::TestArea ta("grid_export");
    {
        const char *test_grid = "TEST.EGRID";
        const char *grid_file;
        if (argc == 1) {
            rd_grid_type *grid =
                rd_grid_alloc_rectangular(4, 4, 2, 1, 1, 1, NULL);
            grid_file = test_grid;
            rd_grid_fwrite_EGRID(grid, grid_file, true);
            rd_grid_free(grid);
        } else
            grid_file = argv[1];

        {
            rd_grid_type *rd_grid = rd_grid_alloc(grid_file);
            if (argc == 1)
                test_assert_true(rd_grid_get_unit_system(rd_grid) ==
                                 RD_METRIC_UNITS);
            rd_file_type *rd_file = rd_file_open(grid_file, 0);

            export_actnum(rd_grid, rd_file);
            export_coord(rd_grid, rd_file);
            export_zcorn(rd_grid, rd_file);
            export_mapaxes(rd_grid, rd_file);
            copy_processed(rd_grid);
            rd_file_close(rd_file);
            rd_grid_free(rd_grid);
        }
    }
}
