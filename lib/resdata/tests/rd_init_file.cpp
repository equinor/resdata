#include <stdlib.h>
#include <stdbool.h>

#include <ert/util/test_util.hpp>
#include <ert/util/test_work_area.hpp>
#include <ert/util/util.h>

#include <resdata/rd_kw.hpp>
#include <resdata/rd_grid.hpp>
#include <resdata/rd_endian_flip.hpp>
#include <resdata/rd_file.hpp>
#include <resdata/rd_init_file.hpp>
#include <resdata/rd_type.hpp>

void test_write_header() {
    int nx = 10;
    int ny = 10;
    int nz = 5;

    rd::util::TestArea ta("WRITE_header");
    int_vector_type *actnum = int_vector_alloc(nx * ny * nz, 1);
    time_t start_time = util_make_date_utc(15, 12, 2010);
    rd_grid_type *rd_grid;

    int_vector_iset(actnum, 10, 0);
    int_vector_iset(actnum, 100, 0);

    rd_grid = rd_grid_alloc_rectangular(nx, ny, nz, 1, 1, 1,
                                        int_vector_get_ptr(actnum));

    // Write poro with global size.
    {
        fortio_type *f = fortio_open_writer("FOO1.INIT", false, RD_ENDIAN_FLIP);
        rd_kw_type *poro =
            rd_kw_alloc("PORO", rd_grid_get_global_size(rd_grid), RD_FLOAT);
        rd_kw_scalar_set_float(poro, 0.10);
        rd_init_file_fwrite_header(f, rd_grid, poro, RD_FIELD_UNITS, 7,
                                   start_time);
        rd_kw_free(poro);
        fortio_fclose(f);
    }

    // Write poro with nactive size.
    {
        fortio_type *f = fortio_open_writer("FOO2.INIT", false, RD_ENDIAN_FLIP);
        rd_kw_type *poro =
            rd_kw_alloc("PORO", rd_grid_get_global_size(rd_grid), RD_FLOAT);
        rd_kw_scalar_set_float(poro, 0.10);
        rd_init_file_fwrite_header(f, rd_grid, poro, RD_FIELD_UNITS, 7,
                                   start_time);
        rd_kw_free(poro);
        fortio_fclose(f);
    }
    {
        rd_file_type *file1 = rd_file_open("FOO1.INIT", 0);
        rd_file_type *file2 = rd_file_open("FOO2.INIT", 0);

        test_assert_true(rd_kw_equal(rd_file_iget_named_kw(file1, "PORV", 0),
                                     rd_file_iget_named_kw(file2, "PORV", 0)));

        rd_file_close(file2);
        rd_file_close(file1);
    }

    // Poro == NULL
    {
        fortio_type *f = fortio_open_writer("FOO3.INIT", false, RD_ENDIAN_FLIP);
        rd_init_file_fwrite_header(f, rd_grid, NULL, RD_METRIC_UNITS, 7,
                                   start_time);
        fortio_fclose(f);
    }
    rd_grid_free(rd_grid);
    int_vector_free(actnum);
}

int main(int argc, char **argv) {
    test_write_header();
    exit(0);
}
