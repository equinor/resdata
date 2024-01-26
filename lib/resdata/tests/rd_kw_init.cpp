#include <stdlib.h>
#include <stdbool.h>
#include <cstring>

#include <ert/util/bool_vector.hpp>
#include <ert/util/test_util.hpp>
#include <ert/util/util.hpp>

#include <resdata/rd_kw.hpp>

void test_int() {
    int N = 1000;
    int i;
    rd_kw_type *kw = rd_kw_alloc("KW", N, RD_INT);
    for (i = 0; i < N; i++)
        test_assert_int_equal(0, rd_kw_iget_int(kw, i));

    rd_kw_free(kw);
}

void test_double() {
    int N = 1000;
    double i;
    rd_kw_type *kw = rd_kw_alloc("KW", N, RD_DOUBLE);
    for (i = 0; i < N; i++)
        test_assert_double_equal(0, rd_kw_iget_double(kw, i));

    rd_kw_free(kw);
}

void test_float() {
    int N = 1000;
    int i;
    rd_kw_type *kw = rd_kw_alloc("KW", N, RD_FLOAT);
    for (i = 0; i < N; i++)
        test_assert_int_equal(0, rd_kw_iget_float(kw, i));

    rd_kw_free(kw);
}

void test_bool() {
    int N = 100;
    bool *data = (bool *)util_malloc(N * sizeof *data);
    rd_kw_type *kw = rd_kw_alloc("BOOL", N, RD_BOOL);
    for (int i = 0; i < N / 2; i++) {
        rd_kw_iset_bool(kw, 2 * i, true);
        rd_kw_iset_bool(kw, 2 * i + 1, false);

        data[2 * i] = true;
        data[2 * i + 1] = false;
    }

    const bool *internal_data = rd_kw_get_bool_ptr(kw);

    test_assert_int_equal(memcmp(internal_data, data, N * sizeof *data), 0);
    rd_kw_free(kw);
    free(data);
}

int main(int argc, char **argv) {
    test_int();
    test_double();
    test_float();
    test_bool();
    exit(0);
}
