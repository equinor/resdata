#include <stdlib.h>
#include <stdbool.h>

#include <ert/util/test_util.hpp>

#include <resdata/rd_grid.hpp>
#include <resdata/rd_file.hpp>
#include <resdata/rd_kw.hpp>
#include <resdata/rd_kw_magic.hpp>
#include <resdata/rd_rsthead.hpp>

void test_file(const char *filename, int occurence, bool exists,
               const rd_rsthead_type *true_header) {
    int report_step = rd_filename_report_nr(filename);
    rd_file_type *rst_file = rd_file_open(filename, 0);
    rd_file_enum file_type = rd_get_file_type(filename, NULL, NULL);
    rd_file_view_type *rst_view;
    rd_rsthead_type *rst_head;

    if (file_type == RD_RESTART_FILE)
        rst_view = rd_file_get_global_view(rst_file);
    else
        rst_view = rd_file_get_restart_view(rst_file, occurence, -1, -1, -1);

    if (exists) {
        test_assert_not_NULL(rst_view);
        rst_head = rd_rsthead_alloc(rst_view, report_step);
        test_assert_not_NULL(rst_head);

        if (occurence == 0) {
            rd_rsthead_type *rst_head0 =
                rd_rsthead_alloc(rst_view, report_step);

            test_assert_true(rd_rsthead_equal(rst_head, rst_head0));
            rd_rsthead_free(rst_head0);
        }
        test_assert_true(rd_rsthead_equal(rst_head, true_header));

        rd_rsthead_free(rst_head);
    } else
        test_assert_NULL(rst_view);
}

int main(int argc, char **argv) {
    rd_rsthead_type true1;
    true1.report_step = 1;
    true1.day = 1;
    true1.year = 2000;
    true1.month = 1;
    true1.sim_time = (time_t)946684800;
    true1.version = 100;
    true1.phase_sum = 7;
    true1.nx = 40;
    true1.ny = 64;
    true1.nz = 14;
    true1.nactive = 34770;
    true1.nwells = 3;
    true1.niwelz = 145;
    true1.nzwelz = 3;
    true1.niconz = 20;
    true1.ncwmax = 120;
    true1.nisegz = 18;
    true1.nsegmx = 1;
    true1.nswlmx = 1;
    true1.nlbrmx = -1;
    true1.nilbrz = -1;
    true1.dualp = 0;
    true1.sim_days = 0;

    rd_rsthead_type true2;
    true2.report_step = 5;
    true2.day = 22;
    true2.year = 1990;
    true2.month = 1;
    true2.sim_time = (time_t)632966400;
    true2.version = 100;
    true2.phase_sum = 7;
    true2.nx = 4;
    true2.ny = 4;
    true2.nz = 4;
    true2.nactive = 64;
    true2.nwells = 3;
    true2.niwelz = 147;
    true2.nzwelz = 3;
    true2.niconz = 20;
    true2.ncwmax = 13;
    true2.nisegz = 18;
    true2.nsegmx = 1;
    true2.nswlmx = 1;
    true2.nlbrmx = -1;
    true2.nilbrz = -1;
    true2.dualp = 1;
    true2.sim_days = 21;

    const char *unified_file = argv[1];
    const char *Xfile = argv[2];

    test_file(unified_file, 0, true, &true1);
    test_file(unified_file, 100, false, NULL);
    test_file(Xfile, 0, true, &true2);

    exit(0);
}
