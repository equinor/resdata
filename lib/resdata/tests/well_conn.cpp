#include <stdlib.h>
#include <stdbool.h>

#include <ert/util/test_util.hpp>
#include <ert/util/stringlist.hpp>
#include <ert/util/util.hpp>

#include <resdata/rd_util.hpp>

#include <resdata/well/well_conn_collection.hpp>
#include <resdata/well/well_conn.hpp>

void test_conn_rate() {
    int i = 10;
    int j = 5;
    int k = 16;
    double CF = 0;
    bool open = true;

    well_conn_dir_enum dir = well_conn_dirX;
    well_conn_type *conn = well_conn_alloc(i, j, k, CF, dir, open);

    test_assert_double_equal(0.0, well_conn_get_oil_rate(conn));
    test_assert_double_equal(0.0, well_conn_get_gas_rate(conn));
    test_assert_double_equal(0.0, well_conn_get_water_rate(conn));
    test_assert_double_equal(0.0, well_conn_get_volume_rate(conn));

    test_assert_double_equal(0.0, well_conn_get_oil_rate_si(conn));
    test_assert_double_equal(0.0, well_conn_get_gas_rate_si(conn));
    test_assert_double_equal(0.0, well_conn_get_water_rate_si(conn));
    test_assert_double_equal(0.0, well_conn_get_volume_rate_si(conn));

    well_conn_free(conn);
}

int main(int argc, char **argv) {
    int i = 10;
    int j = 5;
    int k = 16;
    double CF = 0;
    bool open = true;
    test_install_SIGNALS();

    {
        well_conn_dir_enum dir = well_conn_dirX;
        well_conn_type *conn = well_conn_alloc(i, j, k, CF, dir, open);
        well_conn_type *conn2 = well_conn_alloc(i, j, k, CF, dir, open);
        well_conn_type *conn3 = well_conn_alloc(i, j, k + 1, CF, dir, open);
        test_assert_not_NULL(conn);
        test_assert_true(well_conn_is_instance(conn));
        test_assert_int_equal(i, well_conn_get_i(conn));
        test_assert_int_equal(j, well_conn_get_j(conn));
        test_assert_int_equal(k, well_conn_get_k(conn));
        test_assert_int_equal(dir, well_conn_get_dir(conn));
        test_assert_bool_equal(open, well_conn_open(conn));
        test_assert_false(well_conn_MSW(conn));
        test_assert_true(well_conn_matrix_connection(conn));
        test_assert_true(well_conn_equal(conn, conn2));
        test_assert_false(well_conn_equal(conn, conn3));
        test_assert_double_equal(CF, well_conn_get_connection_factor(conn));
        well_conn_free(conn3);
        well_conn_free(conn2);
        well_conn_free(conn);
    }

    {
        well_conn_dir_enum dir = well_conn_fracX;
        well_conn_type *conn = well_conn_alloc(i, j, k, CF, dir, open);
        test_assert_NULL(conn);
    }

    {
        well_conn_dir_enum dir = well_conn_fracX;
        well_conn_type *conn = well_conn_alloc_fracture(i, j, k, CF, dir, open);
        test_assert_not_NULL(conn);
        test_assert_int_equal(i, well_conn_get_i(conn));
        test_assert_int_equal(j, well_conn_get_j(conn));
        test_assert_int_equal(k, well_conn_get_k(conn));
        test_assert_bool_equal(open, well_conn_open(conn));
        test_assert_int_equal(dir, well_conn_get_dir(conn));
        test_assert_false(well_conn_MSW(conn));
        test_assert_false(well_conn_matrix_connection(conn));
        test_assert_true(well_conn_fracture_connection(conn));
        well_conn_free(conn);
    }

    {
        well_conn_dir_enum dir = well_conn_dirX;
        well_conn_type *conn = well_conn_alloc_fracture(i, j, k, CF, dir, open);
        test_assert_not_NULL(conn);
        well_conn_free(conn);
    }

    {
        int segment = 16;
        well_conn_dir_enum dir = well_conn_dirX;
        well_conn_type *conn =
            well_conn_alloc_MSW(i, j, k, CF, dir, open, segment);
        test_assert_not_NULL(conn);
        test_assert_int_equal(i, well_conn_get_i(conn));
        test_assert_int_equal(j, well_conn_get_j(conn));
        test_assert_int_equal(k, well_conn_get_k(conn));
        test_assert_int_equal(segment, well_conn_get_segment_id(conn));
        test_assert_bool_equal(open, well_conn_open(conn));
        test_assert_int_equal(dir, well_conn_get_dir(conn));
        test_assert_true(well_conn_MSW(conn));
        test_assert_true(well_conn_matrix_connection(conn));
        well_conn_free(conn);
    }

    {
        int segment = 16;
        well_conn_dir_enum dir = well_conn_fracX;
        well_conn_type *conn =
            well_conn_alloc_fracture_MSW(i, j, k, CF, dir, open, segment);
        test_assert_not_NULL(conn);
        test_assert_int_equal(i, well_conn_get_i(conn));
        test_assert_int_equal(j, well_conn_get_j(conn));
        test_assert_int_equal(k, well_conn_get_k(conn));
        test_assert_int_equal(segment, well_conn_get_segment_id(conn));
        test_assert_bool_equal(open, well_conn_open(conn));
        test_assert_int_equal(dir, well_conn_get_dir(conn));
        test_assert_true(well_conn_MSW(conn));
        test_assert_false(well_conn_matrix_connection(conn));
        well_conn_free(conn);
    }

    test_conn_rate();
}
