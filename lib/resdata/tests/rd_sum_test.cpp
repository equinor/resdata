#include <cstdlib>
#include <ctime>

#include <ert/util/test_util.hpp>
#include <ert/util/util.hpp>

#include <resdata/rd_sum.hpp>
#include <resdata/rd_util.hpp>

void test_time_range(const rd_sum_type *rd_sum) {
    // Hardcoded Gurbat case values
    time_t start = rd_make_date(1, 1, 2000);
    time_t end = rd_make_date(31, 12, 2004);

    test_assert_time_t_equal(rd_sum_get_start_time(rd_sum), start);
    test_assert_time_t_equal(rd_sum_get_end_time(rd_sum), end);
    test_assert_time_t_equal(rd_sum_get_data_start(rd_sum), start);
}

int main(int argc, char **argv) {
    const char *case1 = argv[1];

    rd_sum_ptr rd_sum1 = read_summary(case1);

    test_assert_true(rd_sum_is_instance(rd_sum1.get()));
    test_time_range(rd_sum1.get());
    exit(0);
}
