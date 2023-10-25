
#include <ert/util/test_util.hpp>

#include <resdata/rd_sum.hpp>
#include <resdata/smspec_node.hpp>
#include <resdata/rd_sum_tstep.hpp>

rd_sum_type *test_alloc_rd_sum() {
    time_t start_time = util_make_date_utc(1, 1, 2010);
    rd_sum_type *rd_sum = rd_sum_alloc_writer("/tmp/CASE", false, true, ":",
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
    return rd_sum;
}

void test_correct_time_vector() {

    rd_sum_type *rd_sum = test_alloc_rd_sum();
    time_t_vector_type *t = time_t_vector_alloc(0, 0);
    time_t_vector_append(t, util_make_date_utc(2, 1, 2010));
    time_t_vector_append(t, util_make_date_utc(4, 1, 2010));
    time_t_vector_append(t, util_make_date_utc(6, 1, 2010));
    time_t_vector_append(t, util_make_date_utc(8, 1, 2010));
    rd_sum_type *rd_sum_resampled =
        rd_sum_alloc_resample(rd_sum, "kk", t, false, false);
    test_assert_int_equal(rd_sum_get_report_time(rd_sum_resampled, 2),
                          util_make_date_utc(6, 1, 2010));

    const rd_smspec_type *smspec_resampled =
        rd_sum_get_smspec(rd_sum_resampled);
    const rd::smspec_node &node1 =
        rd_smspec_iget_node_w_params_index(smspec_resampled, 1);
    const rd::smspec_node &node2 =
        rd_smspec_iget_node_w_params_index(smspec_resampled, 2);
    const rd::smspec_node &node3 =
        rd_smspec_iget_node_w_params_index(smspec_resampled, 3);
    test_assert_string_equal("BPR", smspec_node_get_keyword(&node2));
    test_assert_string_equal("BARS", smspec_node_get_unit(&node2));

    test_assert_double_equal(
        3.33333, rd_sum_get_from_sim_time(
                     rd_sum_resampled, util_make_date_utc(6, 1, 2010), &node1));
    test_assert_double_equal(
        3.33333, rd_sum_get_from_sim_time(
                     rd_sum_resampled, util_make_date_utc(2, 1, 2010), &node2));
    test_assert_double_equal(
        10.0000, rd_sum_get_from_sim_time(
                     rd_sum_resampled, util_make_date_utc(4, 1, 2010), &node3));

    rd_sum_free(rd_sum_resampled);
    time_t_vector_free(t);
    rd_sum_free(rd_sum);
}

void test_resample_extrapolate_rate() {

    rd_sum_type *rd_sum = test_alloc_rd_sum();

    time_t_vector_type *t = time_t_vector_alloc(0, 0);
    time_t_vector_append(t, util_make_date_utc(1, 1, 2009));
    time_t_vector_append(t, util_make_date_utc(4, 1, 2010));
    time_t_vector_append(t, util_make_date_utc(12, 1, 2010));

    rd_sum_type *rd_sum_resampled =
        rd_sum_alloc_resample(rd_sum, "kk", t, true, true);

    const rd_smspec_type *smspec_resampled =
        rd_sum_get_smspec(rd_sum_resampled);
    const rd::smspec_node &node1 =
        rd_smspec_iget_node_w_params_index(smspec_resampled, 1);
    const rd::smspec_node &node3 =
        rd_smspec_iget_node_w_params_index(smspec_resampled, 3);

    //testing extrapolation for rate wrt. 3 dates: lower, inside and upper
    test_assert_double_equal(
        0, rd_sum_get_from_sim_time(rd_sum_resampled,
                                    util_make_date_utc(1, 1, 2009), &node3));
    test_assert_double_equal(
        10.000, rd_sum_get_from_sim_time(
                    rd_sum_resampled, util_make_date_utc(4, 1, 2010), &node3));
    test_assert_double_equal(
        0, rd_sum_get_from_sim_time(rd_sum_resampled,
                                    util_make_date_utc(12, 1, 2010), &node3));

    //testing extrapolation for variable wrt. 3 dates: lower, inside and upper
    test_assert_double_equal(
        0, rd_sum_get_from_sim_time(rd_sum_resampled,
                                    util_make_date_utc(1, 1, 2009), &node1));
    test_assert_double_equal(
        2.000, rd_sum_get_from_sim_time(
                   rd_sum_resampled, util_make_date_utc(4, 1, 2010), &node1));
    test_assert_double_equal(
        6.000, rd_sum_get_from_sim_time(
                   rd_sum_resampled, util_make_date_utc(12, 1, 2010), &node1));

    rd_sum_free(rd_sum_resampled);
    time_t_vector_free(t);
    rd_sum_free(rd_sum);
}

void test_not_sorted() {
    rd_sum_type *rd_sum = test_alloc_rd_sum();
    time_t_vector_type *t = time_t_vector_alloc(0, 0);
    time_t_vector_append(t, util_make_date_utc(1, 1, 2010));
    time_t_vector_append(t, util_make_date_utc(3, 1, 2010));
    time_t_vector_append(t, util_make_date_utc(2, 1, 2010));
    test_assert_NULL(rd_sum_alloc_resample(rd_sum, "kk", t, false, false));
    time_t_vector_free(t);
    rd_sum_free(rd_sum);
}

int main() {
    test_correct_time_vector();
    test_resample_extrapolate_rate();
    test_not_sorted();
    return 0;
}
