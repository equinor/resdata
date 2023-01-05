#include <stdlib.h>
#include <stdbool.h>

#include <ert/util/test_util.hpp>
#include <ert/util/test_work_area.hpp>
#include <ert/util/util.h>

#include <ert/ecl/ecl_kw.hpp>
#include <ert/ecl/ecl_grid.hpp>
#include <ert/ecl/ecl_endian_flip.hpp>
#include <ert/ecl/ecl_file.hpp>
#include <ert/ecl/ecl_init_file.hpp>
#include <ert/ecl/ecl_type.hpp>

void test_write_header() {
    int nx = 10;
    int ny = 10;
    int nz = 5;

    ecl::util::TestArea ta("WRITE_header");
    int_vector_type *actnum = int_vector_alloc(nx * ny * nz, 1);
    time_t start_time = util_make_date_utc(15, 12, 2010);
    ecl_grid_type *ecl_grid;

    int_vector_iset(actnum, 10, 0);
    int_vector_iset(actnum, 100, 0);

    ecl_grid = ecl_grid_alloc_rectangular(nx, ny, nz, 1, 1, 1,
                                          int_vector_get_ptr(actnum));

    // Write poro with global size.
    {
        fortio_type *f =
            fortio_open_writer("FOO1.INIT", false, ECL_ENDIAN_FLIP);
        ecl_kw_type *poro =
            ecl_kw_alloc("PORO", ecl_grid_get_global_size(ecl_grid), ECL_FLOAT);
        ecl_kw_scalar_set_float(poro, 0.10);
        ecl_init_file_fwrite_header(f, ecl_grid, poro, ECL_FIELD_UNITS, 7,
                                    start_time);
        ecl_kw_free(poro);
        fortio_fclose(f);
    }

    // Write poro with nactive size.
    {
        fortio_type *f =
            fortio_open_writer("FOO2.INIT", false, ECL_ENDIAN_FLIP);
        ecl_kw_type *poro =
            ecl_kw_alloc("PORO", ecl_grid_get_global_size(ecl_grid), ECL_FLOAT);
        ecl_kw_scalar_set_float(poro, 0.10);
        ecl_init_file_fwrite_header(f, ecl_grid, poro, ECL_FIELD_UNITS, 7,
                                    start_time);
        ecl_kw_free(poro);
        fortio_fclose(f);
    }
    {
        ecl_file_type *file1 = ecl_file_open("FOO1.INIT", 0);
        ecl_file_type *file2 = ecl_file_open("FOO2.INIT", 0);

        test_assert_true(
            ecl_kw_equal(ecl_file_iget_named_kw(file1, "PORV", 0),
                         ecl_file_iget_named_kw(file2, "PORV", 0)));

        ecl_file_close(file2);
        ecl_file_close(file1);
    }

    // Poro == NULL
    {
        fortio_type *f =
            fortio_open_writer("FOO3.INIT", false, ECL_ENDIAN_FLIP);
        ecl_init_file_fwrite_header(f, ecl_grid, NULL, ECL_METRIC_UNITS, 7,
                                    start_time);
        fortio_fclose(f);
    }
    ecl_grid_free(ecl_grid);
    int_vector_free(actnum);
}

int main(int argc, char **argv) {
    test_write_header();
    exit(0);
}
