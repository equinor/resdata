#include <cstdlib>
#include <ctime>

#include <ert/util/test_util.hpp>
#include <ert/util/util.hpp>

#include <resdata/rd_util.hpp>

void test_date(int mday, int month, int year) {
    time_t t0 = rd_make_date(mday, month, year);
    time_t t1 = util_make_date_utc(mday, month, year);

    test_assert_time_t_equal(t0, t1);
}

int main(int argc, char **argv) {
    test_date(10, 10, 2000);
    exit(0);
}
