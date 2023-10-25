#include <vector>

#include <ert/util/test_work_area.hpp>
#include <ert/util/test_util.hpp>

#include <resdata/rd_sum.hpp>

#include "detail/resdata/rd_unsmry_loader.hpp"

rd_sum_type *write_rd_sum() {
    time_t start_time = util_make_date_utc(1, 1, 2010);
    rd_sum_type *rd_sum = rd_sum_alloc_writer("CASE", false, true, ":",
                                              start_time, true, 10, 10, 10);
    double sim_seconds = 0;

    int num_dates = 4;
    double ministep_length = 86400; // seconds in a day

    const rd::smspec_node *node1 =
        rd_sum_add_var(rd_sum, "FOPT", NULL, 0, "Barrels", 99.0);
    const rd::smspec_node *node2 =
        rd_sum_add_var(rd_sum, "BPR", NULL, 567, "BARS", 0.0);
    const rd::smspec_node *node3 =
        rd_sum_add_var(rd_sum, "WWCT", "OP-1", 0, "(1)", 0.0);

    for (int report_step = 0; report_step < num_dates; report_step++) {
        {
            rd_sum_tstep_type *tstep =
                rd_sum_add_tstep(rd_sum, report_step + 1, sim_seconds);
            rd_sum_tstep_set_from_node(tstep, *node1, report_step * 2.0);
            rd_sum_tstep_set_from_node(tstep, *node2, report_step * 4.0 + 2.0);
            rd_sum_tstep_set_from_node(tstep, *node3, report_step * 6.0 + 4.0);
        }
        sim_seconds += ministep_length * 3;
    }
    rd_sum_fwrite(rd_sum);
    return rd_sum;
}

void test_load() {
    rd::util::TestArea ta("rd_sum_loader");
    rd_sum_type *rd_sum = write_rd_sum();
    test_assert_true(util_file_exists("CASE.SMSPEC"));
    test_assert_true(util_file_exists("CASE.UNSMRY"));
    rd::unsmry_loader *loader =
        new rd::unsmry_loader(rd_sum_get_smspec(rd_sum), "CASE.UNSMRY", 0);

    const std::vector<double> FOPT_value = loader->get_vector(1);
    const std::vector<double> BPR_value = loader->get_vector(2);
    const std::vector<double> WWCT_value = loader->get_vector(3);
    test_assert_int_equal(FOPT_value.size(), 4);
    test_assert_double_equal(FOPT_value[3], 6.0);
    test_assert_double_equal(BPR_value[2], 10.0);
    test_assert_double_equal(WWCT_value[1], 10.0);

    delete loader;
    rd_sum_free(rd_sum);
}

int main() {
    test_load();
    return 0;
}
