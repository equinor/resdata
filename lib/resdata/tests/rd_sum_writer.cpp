#include <stdlib.h>
#include <stdbool.h>
#include <stdexcept>

#include <ert/util/test_util.hpp>
#include <ert/util/time_t_vector.hpp>
#include <ert/util/util.h>
#include <ert/util/test_work_area.hpp>

#include <resdata/rd_sum.hpp>
#include <resdata/rd_grid.hpp>
#include <resdata/rd_file.hpp>
#include <resdata/rd_kw_magic.hpp>

double write_summary(const char *name, time_t start_time, int nx, int ny,
                     int nz, int num_dates, int num_ministep,
                     double ministep_length) {
    rd_sum_type *rd_sum = rd_sum_alloc_writer(name, false, true, ":",
                                              start_time, true, nx, ny, nz);
    double sim_seconds = 0;
    rd_smspec_type *smspec = rd_sum_get_smspec(rd_sum);

    test_assert_int_equal(rd_smspec_get_params_size(smspec), 1);
    const rd::smspec_node *node1 =
        rd_smspec_add_node(smspec, "FOPT", "Barrels", 99.0);
    const rd::smspec_node *node2 =
        rd_smspec_add_node(smspec, "BPR", 567, "BARS", 0.0);
    const rd::smspec_node *node3 =
        rd_smspec_add_node(smspec, "WWCT", "OP-1", "(1)", 0.0);
    test_assert_int_equal(rd_smspec_get_params_size(smspec), 4);

    for (int report_step = 0; report_step < num_dates; report_step++) {
        for (int step = 0; step < num_ministep; step++) {
            /* Simulate .... */

            {
                rd_sum_tstep_type *tstep =
                    rd_sum_add_tstep(rd_sum, report_step + 1, sim_seconds);
                rd_sum_tstep_set_from_node(tstep, *node1, sim_seconds);
                rd_sum_tstep_set_from_node(tstep, *node2, 10 * sim_seconds);
                rd_sum_tstep_set_from_node(tstep, *node3, 100 * sim_seconds);

                test_assert_double_equal(
                    rd_sum_tstep_get_from_node(tstep, *node1), sim_seconds);
                test_assert_double_equal(
                    rd_sum_tstep_get_from_node(tstep, *node2),
                    sim_seconds * 10);
                test_assert_double_equal(
                    rd_sum_tstep_get_from_node(tstep, *node3),
                    sim_seconds * 100);
            }
            sim_seconds += ministep_length;
        }
    }
    rd_sum_fwrite(rd_sum);
    rd_sum_free(rd_sum);
    return sim_seconds;
}

int write_restart_summary(const char *name, const char *restart_name,
                          int start_report_step, double sim_seconds,
                          time_t start_time, int nx, int ny, int nz,
                          int num_dates, int num_ministep,
                          double ministep_length) {
    rd_sum_type *rd_sum = rd_sum_alloc_restart_writer(
        name, restart_name, false, true, ":", start_time, true, nx, ny, nz);
    rd_smspec_type *smspec = rd_sum_get_smspec(rd_sum);

    const rd::smspec_node *node1 =
        rd_smspec_add_node(smspec, "FOPT", "Barrels", 99.0);
    const rd::smspec_node *node2 =
        rd_smspec_add_node(smspec, "BPR", 567, "BARS", 0.0);
    const rd::smspec_node *node3 =
        rd_smspec_add_node(smspec, "WWCT", "OP-1", "(1)", 0.0);

    int num_report_steps = start_report_step + num_dates;
    for (int report_step = start_report_step; report_step < num_report_steps;
         report_step++) {
        for (int step = 0; step < num_ministep; step++) {
            {
                rd_sum_tstep_type *tstep =
                    rd_sum_add_tstep(rd_sum, report_step + 1, sim_seconds);
                rd_sum_tstep_set_from_node(tstep, *node1, sim_seconds);
                rd_sum_tstep_set_from_node(tstep, *node2, 10 * sim_seconds);
                rd_sum_tstep_set_from_node(tstep, *node3, 100 * sim_seconds);
            }
            sim_seconds += ministep_length;
        }
    }
    rd_sum_fwrite(rd_sum);
    rd_sum_free(rd_sum);
    return sim_seconds;
}

void test_write_read() {
    const char *name = "CASE";
    time_t start_time = util_make_date_utc(1, 1, 2010);
    time_t end_time = start_time;
    int nx = 10;
    int ny = 11;
    int nz = 12;
    int num_dates = 5;
    int num_ministep = 10;
    double ministep_length =
        86400; // Seconds - numerical value chosen to avoid rounding problems when converting between seconds and days.
    {
        rd::util::TestArea ta("write_read");
        rd_sum_type *rd_sum;

        write_summary(name, start_time, nx, ny, nz, num_dates, num_ministep,
                      ministep_length);
        rd_sum = rd_sum_fread_alloc_case(name, ":");
        test_assert_true(rd_sum_is_instance(rd_sum));
        test_assert_true(rd_sum_get_start_time(rd_sum) == start_time);

        /* Time direction */
        test_assert_time_t_equal(start_time, rd_sum_get_start_time(rd_sum));
        test_assert_time_t_equal(start_time, rd_sum_get_data_start(rd_sum));
        util_inplace_forward_seconds_utc(
            &end_time, (num_dates * num_ministep - 1) * ministep_length);
        test_assert_time_t_equal(end_time, rd_sum_get_end_time(rd_sum));

        /* Keys */
        test_assert_true(rd_sum_has_key(rd_sum, "FOPT"));
        test_assert_true(rd_sum_has_key(rd_sum, "WWCT:OP-1"));
        test_assert_true(rd_sum_has_key(rd_sum, "BPR:567"));
        {
            rd_grid_type *grid =
                rd_grid_alloc_rectangular(nx, ny, nz, 1, 1, 1, NULL);
            int i, j, k;
            char *ijk_key;
            rd_grid_get_ijk1(grid, 567 - 1, &i, &j, &k);
            ijk_key = util_alloc_sprintf("BPR:%d,%d,%d", i + 1, j + 1, k + 1);

            free(ijk_key);
            rd_grid_free(grid);
        }

        test_assert_throw(rd_sum_get_general_var(rd_sum, 1, "NO_SUCH_KEY"),
                          std::out_of_range);
        rd_sum_free(rd_sum);
    }
}

void test_rd_sum_alloc_restart_writer() {
    rd::util::TestArea ta("sum_write_restart");
    {
        const char *name1 = "CASE1";
        const char *name2 = "CASE2";
        time_t start_time = util_make_date_utc(1, 1, 2010);
        int nx = 10;
        int ny = 11;
        int nz = 12;
        int num_dates = 5;
        int num_ministep = 10;
        double ministep_length = 36000; // Seconds

        int sim_seconds =
            write_summary(name1, start_time, nx, ny, nz, num_dates,
                          num_ministep, ministep_length);
        sim_seconds = write_restart_summary(
            name2, name1, num_dates, sim_seconds, start_time, nx, ny, nz,
            num_dates, num_ministep, ministep_length);

        rd_sum_type *case1 = rd_sum_fread_alloc_case(name1, ":");
        rd_sum_type *case2 = rd_sum_fread_alloc_case(name2, ":");
        test_assert_true(rd_sum_is_instance(case2));

        test_assert_true(rd_sum_has_key(case2, "FOPT"));

        rd_file_type *restart_file = rd_file_open("CASE2.SMSPEC", 0);
        rd_file_view_type *view_file = rd_file_get_global_view(restart_file);
        test_assert_true(rd_file_view_has_kw(view_file, RESTART_KW));
        rd_kw_type *restart_kw =
            rd_file_view_iget_named_kw(view_file, "RESTART", 0);
        test_assert_int_equal(8, rd_kw_get_size(restart_kw));
        test_assert_string_equal("CASE1   ",
                                 (const char *)rd_kw_iget_ptr(restart_kw, 0));
        test_assert_string_equal("        ",
                                 (const char *)rd_kw_iget_ptr(restart_kw, 1));

        for (int time_index = 0; time_index < rd_sum_get_data_length(case1);
             time_index++)
            test_assert_double_equal(
                rd_sum_get_general_var(case1, time_index, "FOPT"),
                rd_sum_get_general_var(case2, time_index, "FOPT"));

        rd_sum_free(case2);
        rd_sum_free(case1);
        rd_file_close(restart_file);
    }
}

void test_long_restart_names() {
    char restart_case[65] = {0};
    for (int n = 0; n < 8; n++) {
        char s[9];
        sprintf(s, "WWWWGGG%d", n);
        strcat(restart_case, s);
    }
    const char *name = "THE_CASE";
    rd::util::TestArea ta("suM_write_restart_long_name");
    {
        int restart_step = 77;
        time_t start_time = util_make_date_utc(1, 1, 2010);
        rd_sum_type *rd_sum = rd_sum_alloc_restart_writer2(
            name, restart_case, restart_step, false, true, ":", start_time,
            true, 3, 3, 3);
        rd_sum_fwrite(rd_sum);
        rd_sum_free(rd_sum);

        rd_file_type *smspec_file = rd_file_open("THE_CASE.SMSPEC", 0);
        rd_file_view_type *view_file = rd_file_get_global_view(smspec_file);
        test_assert_true(rd_file_view_has_kw(view_file, RESTART_KW));
        rd_kw_type *restart_kw =
            rd_file_view_iget_named_kw(view_file, "RESTART", 0);
        test_assert_int_equal(8, rd_kw_get_size(restart_kw));

        for (int n = 0; n < 8; n++) {
            char s[9];
            sprintf(s, "WWWWGGG%d", n);
            test_assert_string_equal(s, rd_kw_iget_char_ptr(restart_kw, n));
        }
        rd_file_close(smspec_file);
        {
            rd_smspec_type *smspec = rd_smspec_alloc_restart_writer(
                ":",
                "ABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJ"
                "KLMNOPQRSTUVWXYZ",
                10, start_time, true, 3, 3, 3);
            /*
           Restart case is too long - it is ignored.
         */
            test_assert_NULL(rd_smspec_get_restart_case(smspec));
            rd_smspec_free(smspec);
        }
    }
}

int main(int argc, char **argv) {
    util_install_signals();
    test_write_read();
    test_rd_sum_alloc_restart_writer();
    test_long_restart_names();
    exit(0);
}
