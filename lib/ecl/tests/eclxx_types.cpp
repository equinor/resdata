#include <stdexcept>
#include <fstream>

#include <ert/util/test_util.hpp>
#include <ert/ecl/ecl_type.hpp>
#include <ert/ecl/FortIO.hpp>

void test_ECL_INT() {
    ecl_data_type dt = ECL_INT;
    test_assert_int_equal(dt.element_size, sizeof(int));
    test_assert_int_equal(dt.type, ECL_INT_TYPE);
}

void test_ECL_FLOAT() {
    ecl_data_type dt = ECL_FLOAT;
    test_assert_int_equal(dt.element_size, sizeof(float));
    test_assert_int_equal(dt.type, ECL_FLOAT_TYPE);
}

void test_ECL_DOUBLE() {
    ecl_data_type dt = ECL_DOUBLE;
    test_assert_int_equal(dt.element_size, sizeof(double));
    test_assert_int_equal(dt.type, ECL_DOUBLE_TYPE);
}

void test_ECL_CHAR() {
    ecl_data_type dt = ECL_CHAR;
    test_assert_int_equal(dt.element_size, ECL_STRING8_LENGTH + 1);
    test_assert_int_equal(dt.type, ECL_CHAR_TYPE);
}

int main(int argc, char **argv) {
    test_ECL_INT();
    test_ECL_FLOAT();
    test_ECL_DOUBLE();
    test_ECL_CHAR();
}
