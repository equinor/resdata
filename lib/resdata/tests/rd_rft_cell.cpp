#include <stdlib.h>
#include <stdbool.h>

#include <ert/util/test_util.hpp>
#include <ert/util/time_t_vector.hpp>
#include <ert/util/util.hpp>

#include <resdata/rd_rft_node.hpp>
#include <resdata/rd_rft_cell.hpp>

void test_rft_cell() {
    const int i = 10;
    const int j = 11;
    const int k = 12;

    const double depth = 100;
    const double pressure = 200;
    const double swat = 0.25;
    const double sgas = 0.35;

    rd_rft_cell_type *cell =
        rd_rft_cell_alloc_RFT(i, j, k, depth, pressure, swat, sgas);

    test_assert_int_equal(i, rd_rft_cell_get_i(cell));
    test_assert_int_equal(j, rd_rft_cell_get_j(cell));
    test_assert_int_equal(k, rd_rft_cell_get_k(cell));

    {
        int ii, jj, kk;
        rd_rft_cell_get_ijk(cell, &ii, &jj, &kk);
        test_assert_int_equal(i, ii);
        test_assert_int_equal(j, jj);
        test_assert_int_equal(k, kk);
    }

    test_assert_double_equal(depth, rd_rft_cell_get_depth(cell));
    test_assert_double_equal(pressure, rd_rft_cell_get_pressure(cell));
    test_assert_double_equal(swat, rd_rft_cell_get_swat(cell));
    test_assert_double_equal(sgas, rd_rft_cell_get_sgas(cell));
    test_assert_double_equal(1 - (swat + sgas), rd_rft_cell_get_soil(cell));

    test_assert_double_equal(RD_RFT_CELL_INVALID_VALUE,
                             rd_rft_cell_get_orat(cell));
    test_assert_double_equal(RD_RFT_CELL_INVALID_VALUE,
                             rd_rft_cell_get_grat(cell));
    test_assert_double_equal(RD_RFT_CELL_INVALID_VALUE,
                             rd_rft_cell_get_wrat(cell));
    test_assert_double_equal(RD_RFT_CELL_INVALID_VALUE,
                             rd_rft_cell_get_flowrate(cell));
    test_assert_double_equal(RD_RFT_CELL_INVALID_VALUE,
                             rd_rft_cell_get_connection_start(cell));
    test_assert_double_equal(RD_RFT_CELL_INVALID_VALUE,
                             rd_rft_cell_get_connection_end(cell));
    test_assert_double_equal(RD_RFT_CELL_INVALID_VALUE,
                             rd_rft_cell_get_oil_flowrate(cell));
    test_assert_double_equal(RD_RFT_CELL_INVALID_VALUE,
                             rd_rft_cell_get_gas_flowrate(cell));
    test_assert_double_equal(RD_RFT_CELL_INVALID_VALUE,
                             rd_rft_cell_get_water_flowrate(cell));

    rd_rft_cell_free(cell);
    {
        rd_rft_cell_type *cell =
            rd_rft_cell_alloc_RFT(i, j, k, depth, pressure, swat, sgas);

        test_assert_true(rd_rft_cell_ijk_equal(cell, i, j, k));
        test_assert_false(rd_rft_cell_ijk_equal(cell, i, j, k + 1));

        rd_rft_cell_free__(cell);
    }
}

void test_plt_cell() {
    const int i = 10;
    const int j = 11;
    const int k = 12;

    const double depth = 100;
    const double pressure = 200;
    const double orat = 0.25;
    const double grat = 0.35;
    const double wrat = 0.45;
    const double flowrate = 100.0;
    const double connection_start = 891;
    const double connection_end = 979;
    const double oil_flowrate = 0.891;
    const double gas_flowrate = 7771;
    const double water_flowrate = 77614;

    rd_rft_cell_type *cell = rd_rft_cell_alloc_PLT(
        i, j, k, depth, pressure, orat, grat, wrat, connection_start,
        connection_end, flowrate, oil_flowrate, gas_flowrate, water_flowrate);

    test_assert_int_equal(i, rd_rft_cell_get_i(cell));
    test_assert_int_equal(j, rd_rft_cell_get_j(cell));
    test_assert_int_equal(k, rd_rft_cell_get_k(cell));

    {
        int ii, jj, kk;
        rd_rft_cell_get_ijk(cell, &ii, &jj, &kk);
        test_assert_int_equal(i, ii);
        test_assert_int_equal(j, jj);
        test_assert_int_equal(k, kk);
    }

    test_assert_double_equal(depth, rd_rft_cell_get_depth(cell));
    test_assert_double_equal(pressure, rd_rft_cell_get_pressure(cell));

    test_assert_double_equal(orat, rd_rft_cell_get_orat(cell));
    test_assert_double_equal(grat, rd_rft_cell_get_grat(cell));
    test_assert_double_equal(wrat, rd_rft_cell_get_wrat(cell));
    test_assert_double_equal(connection_start,
                             rd_rft_cell_get_connection_start(cell));
    test_assert_double_equal(connection_end,
                             rd_rft_cell_get_connection_end(cell));
    test_assert_double_equal(flowrate, rd_rft_cell_get_flowrate(cell));

    test_assert_double_equal(oil_flowrate, rd_rft_cell_get_oil_flowrate(cell));
    test_assert_double_equal(gas_flowrate, rd_rft_cell_get_gas_flowrate(cell));
    test_assert_double_equal(water_flowrate,
                             rd_rft_cell_get_water_flowrate(cell));

    test_assert_double_equal(RD_RFT_CELL_INVALID_VALUE,
                             rd_rft_cell_get_swat(cell));
    test_assert_double_equal(RD_RFT_CELL_INVALID_VALUE,
                             rd_rft_cell_get_sgas(cell));
    test_assert_double_equal(RD_RFT_CELL_INVALID_VALUE,
                             rd_rft_cell_get_soil(cell));

    rd_rft_cell_free(cell);
}

int main(int argc, char **argv) {

    test_rft_cell();
    test_plt_cell();
    exit(0);
}
