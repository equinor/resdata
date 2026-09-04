#include <cstdlib>
#include <cstring>

#include <ert/util/test_util.hpp>
#include <ert/util/util.hpp>

#include <resdata/rd_kw.hpp>
#include <resdata/rd_type.hpp>

void test_int() {
    int N = 1000;
    int i;
    rd::KW kw{"KW", N, RD_INT};
    for (i = 0; i < N; i++)
        test_assert_int_equal(0, kw.at<int>(i));
}

void test_double() {
    int N = 1000;
    double i;
    rd::KW kw{"KW", N, RD_DOUBLE};
    for (i = 0; i < N; i++)
        test_assert_double_equal(0, kw.at<double>(i));
}

void test_float() {
    int N = 1000;
    int i;
    rd::KW kw{"KW", N, RD_FLOAT};
    for (i = 0; i < N; i++)
        test_assert_int_equal(0, kw.at<float>(i));
}

void test_bool() {
    size_t N = 100;
    bool *data = (bool *)util_malloc(N * sizeof *data);
    rd::KW kw{"BOOL", N, RD_BOOL};
    for (size_t i = 0; i < N / 2; i++) {
        kw.at<bool>(2 * i) = true;
        kw.at<bool>(2 * i + 1) = false;

        data[2 * i] = true;
        data[2 * i + 1] = false;
    }

    const char *internal_data = kw.get_vector<char>().data();

    test_assert_int_equal(memcmp(internal_data, data, N * sizeof *data), 0);
    free(data);
}

int main(int argc, char **argv) {
    test_int();
    test_double();
    test_float();
    test_bool();
    exit(0);
}
