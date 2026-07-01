#include <cstdlib>

#include <ert/util/test_util.hpp>
#include <ert/util/stringlist.hpp>
#include <ert/util/util.hpp>

#include <resdata/rd_util.hpp>

#include <resdata/well/well_conn.hpp>

void test_conn_rate() {
    int i = 10;
    int j = 5;
    int k = 16;
    double CF = 0;
    bool open = true;

    auto dir = WellConnDir::X;
    WellConnection well_conn(i, j, k, CF, dir, open);

    test_assert_double_equal(0.0, well_conn.get_oil_rate());
    test_assert_double_equal(0.0, well_conn.get_gas_rate());
    test_assert_double_equal(0.0, well_conn.get_water_rate());
    test_assert_double_equal(0.0, well_conn.get_volume_rate());

    test_assert_double_equal(0.0, well_conn.get_oil_rate_si());
    test_assert_double_equal(0.0, well_conn.get_gas_rate_si());
    test_assert_double_equal(0.0, well_conn.get_water_rate_si());
    test_assert_double_equal(0.0, well_conn.get_volume_rate_si());
}

int main(int argc, char **argv) {
    int i = 10;
    int j = 5;
    int k = 16;
    double CF = 0;
    bool open = true;
    {
        auto dir = WellConnDir::X;
        WellConnection conn(i, j, k, CF, dir, open);
        WellConnection conn2(i, j, k, CF, dir, open);
        WellConnection conn3(i, j, k + 1, CF, dir, open);
        test_assert_int_equal(i, conn.get_i());
        test_assert_int_equal(j, conn.get_j());
        test_assert_int_equal(k, conn.get_k());
        test_assert_true(dir == conn.get_dir());
        test_assert_bool_equal(open, conn.is_open());
        test_assert_false(conn.is_MSW());
        test_assert_true(conn.is_matrix_connection());
        test_assert_true(conn == conn2);
        test_assert_false(conn == conn3);
        test_assert_double_equal(CF, conn.get_connection_factor());
    }

    test_assert_throw(WellConnection(i, j, k, CF, WellConnDir::fracX, open),
                      InvalidDirection);

    {
        auto dir = WellConnDir::X;
        WellConnection conn(i, j, k, CF, dir, open,
                            WELL_CONN_NORMAL_WELL_SEGMENT_ID, false,
                            RD_METRIC_UNITS);
        test_assert_int_equal(i, conn.get_i());
        test_assert_int_equal(j, conn.get_j());
        test_assert_int_equal(k, conn.get_k());
        test_assert_bool_equal(open, conn.is_open());
        test_assert_true(dir == conn.get_dir());
        test_assert_false(conn.is_MSW());
        test_assert_false(conn.is_matrix_connection());
        test_assert_true(conn.is_fracture_connection());
    }

    {
        int segment = 16;
        auto dir = WellConnDir::X;
        WellConnection conn(i, j, k, CF, dir, open, segment, true,
                            RD_METRIC_UNITS);
        test_assert_int_equal(i, conn.get_i());
        test_assert_int_equal(j, conn.get_j());
        test_assert_int_equal(k, conn.get_k());
        test_assert_int_equal(segment, conn.get_segment_id());
        test_assert_bool_equal(open, conn.is_open());
        test_assert_true(dir == conn.get_dir());
        test_assert_true(conn.is_MSW());
        test_assert_true(conn.is_matrix_connection());
    }

    {
        int segment = 16;
        auto dir = WellConnDir::X;
        WellConnection conn(i, j, k, CF, dir, open, segment, false,
                            RD_METRIC_UNITS);
        test_assert_int_equal(i, conn.get_i());
        test_assert_int_equal(j, conn.get_j());
        test_assert_int_equal(k, conn.get_k());
        test_assert_int_equal(segment, conn.get_segment_id());
        test_assert_bool_equal(open, conn.is_open());
        test_assert_true(dir == conn.get_dir());
        test_assert_true(conn.is_MSW());
        test_assert_false(conn.is_matrix_connection());
    }

    test_conn_rate();
}
