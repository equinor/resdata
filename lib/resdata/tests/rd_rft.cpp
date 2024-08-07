#include <stdlib.h>
#include <stdbool.h>
#include <cstring>

#include <ert/util/test_util.hpp>
#include <ert/util/time_t_vector.hpp>
#include <ert/util/util.hpp>

#include <resdata/rd_rft_file.hpp>
#include <ert/util/vector.hpp>
#include <ert/util/int_vector.hpp>
#include <ert/util/test_work_area.hpp>
#include <resdata/rd_rft_node.hpp>

void test_rft_read_write(const char *rft_file) {
    rd_rft_file_type *rft = rd_rft_file_alloc(rft_file);
    rd_rft_node_type **nodes =
        (rd_rft_node_type **)malloc(sizeof(rd_rft_node_type *) * 3);
    int size = rd_rft_file_get_size(rft);
    for (int i = 0; i < size; i++) {
        rd_rft_node_type *rft_node = rd_rft_file_iget_node(rft, i);
        nodes[i] = rft_node;
    }
    rd_rft_node_type *old_node = rd_rft_file_iget_node(rft, 0);
    rd_rft_node_type *new_node =
        rd_rft_node_alloc_new("DUMMY", "R", rd_rft_node_get_date(old_node),
                              rd_rft_node_get_days(old_node));
    nodes[2] = new_node;
    rd::util::TestArea ta("rft");

    rd_rft_file_update("eclipse.rft", nodes, 3, RD_METRIC_UNITS);
    free(nodes);
}

// Hardcoded GURBAT values
void test_rft(const char *rft_file) {
    rd_rft_file_type *rft = rd_rft_file_alloc(rft_file);
    rd_rft_node_type *rft_node = rd_rft_file_iget_node(rft, 0);

    test_assert_true(rd_rft_node_is_RFT(rft_node));
    test_assert_int_equal(14, rd_rft_node_get_size(rft_node));
    test_assert_false(rd_rft_node_is_MSW(rft_node));

    test_assert_double_equal(260.6111, rd_rft_node_iget_pressure(rft_node, 0));
    test_assert_double_equal(0.0581993, rd_rft_node_iget_soil(rft_node, 0));
    test_assert_double_equal(0.9405648, rd_rft_node_iget_swat(rft_node, 0));
    test_assert_double_equal(0.00123579, rd_rft_node_iget_sgas(rft_node, 0));

    {
        int i, j, k;

        rd_rft_node_iget_ijk(rft_node, 0, &i, &j, &k);
        test_assert_int_equal(32, i);
        test_assert_int_equal(53, j);
        test_assert_int_equal(0, k);

        rd_rft_node_iget_ijk(rft_node, 13, &i, &j, &k);
        test_assert_int_equal(32, i);
        test_assert_int_equal(54, j);
        test_assert_int_equal(12, k);

        for (i = 0; i < rd_rft_node_get_size(rft_node); i++) {
            const rd_rft_cell_type *cell1 = rd_rft_node_iget_cell(rft_node, i);
            const rd_rft_cell_type *cell2 =
                rd_rft_node_iget_cell_sorted(rft_node, i);

            test_assert_ptr_equal(cell1, cell2);
        }
    }
    rd_rft_file_free(rft);
}

void test_plt_msw(const char *plt_file) {
    rd_rft_file_type *plt = rd_rft_file_alloc(plt_file);
    rd_rft_node_type *plt_node = rd_rft_file_iget_node(plt, 11);

    test_assert_true(rd_rft_node_is_PLT(plt_node));
    test_assert_true(rd_rft_node_is_MSW(plt_node));
    test_assert_int_equal(22, rd_rft_node_get_size(plt_node));
    {
        int i;
        for (i = 1; i < rd_rft_node_get_size(plt_node); i++) {
            const rd_rft_cell_type *prev_cell =
                rd_rft_node_iget_cell(plt_node, i - 1);
            const rd_rft_cell_type *this_cell =
                rd_rft_node_iget_cell(plt_node, i);

            test_assert_true(rd_rft_cell_get_connection_start(prev_cell) <
                             rd_rft_cell_get_connection_start(this_cell));
            test_assert_true(rd_rft_cell_get_connection_end(prev_cell) <
                             rd_rft_cell_get_connection_end(this_cell));
        }
    }

    rd_rft_file_free(plt);
}

// Hardcoded values from a test case with a PLT.
void test_plt(const char *plt_file) {
    rd_rft_file_type *plt = rd_rft_file_alloc(plt_file);
    rd_rft_node_type *plt_node = rd_rft_file_iget_node(plt, 11);

    test_assert_true(rd_rft_node_is_PLT(plt_node));
    test_assert_false(rd_rft_node_is_MSW(plt_node));
    test_assert_int_equal(22, rd_rft_node_get_size(plt_node));

    test_assert_double_equal(244.284, rd_rft_node_iget_pressure(plt_node, 0));
    test_assert_double_equal(167.473, rd_rft_node_iget_orat(plt_node, 0));
    test_assert_double_equal(41682.2, rd_rft_node_iget_grat(plt_node, 0));
    test_assert_double_equal(0.958927, rd_rft_node_iget_wrat(plt_node, 0));

    {
        int i, j, k;

        rd_rft_node_iget_ijk(plt_node, 0, &i, &j, &k);
        test_assert_int_equal(39, i);
        test_assert_int_equal(33, j);
        test_assert_int_equal(16, k);

        rd_rft_node_iget_ijk(plt_node, 21, &i, &j, &k);
        test_assert_int_equal(44, i);
        test_assert_int_equal(34, j);
        test_assert_int_equal(7, k);

        for (i = 0; i < rd_rft_node_get_size(plt_node); i++) {
            const rd_rft_cell_type *cell1 = rd_rft_node_iget_cell(plt_node, i);
            const rd_rft_cell_type *cell2 =
                rd_rft_node_iget_cell_sorted(plt_node, i);

            test_assert_ptr_equal(cell1, cell2);
        }
    }

    rd_rft_file_free(plt);
}

void test_simple_load_rft(const char *filename) {
    rd_rft_file_type *rft_file = rd_rft_file_alloc_case(filename);
    rd_rft_file_free(rft_file);
}

int main(int argc, char **argv) {
    const char *rft_file = argv[1];
    const char *mode_string = argv[2];
    util_install_signals();

    if (strcmp(mode_string, "RFT") == 0)
        test_rft(rft_file);
    else if (strcmp(mode_string, "RFT_RW") == 0)
        test_rft_read_write(rft_file);
    else if (strcmp(mode_string, "PLT") == 0)
        test_plt(rft_file);
    else if (strcmp(mode_string, "MSW-PLT") == 0)
        test_plt_msw(rft_file);
    else if (strcmp(mode_string, "SIMPLE") == 0)
        test_simple_load_rft(rft_file);
    else
        test_error_exit(
            "Second argument:%s not recognized. Valid values are: RFT and PLT",
            mode_string);

    exit(0);
}
