#include <stdexcept>
#include <fstream>

#include <ert/util/test_util.hpp>
#include <resdata/rd_type.hpp>
#include <resdata/FortIO.hpp>

void test_RD_INT() {
    rd_data_type dt = RD_INT;
    test_assert_int_equal(dt.element_size, sizeof(int));
    test_assert_int_equal(dt.type, RD_INT_TYPE);
}

void test_RD_FLOAT() {
    rd_data_type dt = RD_FLOAT;
    test_assert_int_equal(dt.element_size, sizeof(float));
    test_assert_int_equal(dt.type, RD_FLOAT_TYPE);
}

void test_RD_DOUBLE() {
    rd_data_type dt = RD_DOUBLE;
    test_assert_int_equal(dt.element_size, sizeof(double));
    test_assert_int_equal(dt.type, RD_DOUBLE_TYPE);
}

void test_RD_CHAR() {
    rd_data_type dt = RD_CHAR;
    test_assert_int_equal(dt.element_size, RD_STRING8_LENGTH + 1);
    test_assert_int_equal(dt.type, RD_CHAR_TYPE);
}

int main(int argc, char **argv) {
    test_RD_INT();
    test_RD_FLOAT();
    test_RD_DOUBLE();
    test_RD_CHAR();
}
