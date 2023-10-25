#include <ert/util/test_util.hpp>
#include <resdata/rd_util.hpp>

int main(int argc, char **argv) {
    test_assert_true(rd_valid_basename_fmt("rd_%d.data"));
    test_assert_true(rd_valid_basename_fmt("RD_%d.DATA"));
    test_assert_true(rd_valid_basename_fmt("RD_%04d.DATA"));
    test_assert_true(rd_valid_basename_fmt("mypath/RD_%04d.DATA"));
    test_assert_true(rd_valid_basename_fmt("MYPATH/RD_%04d.DATA"));
    test_assert_true(rd_valid_basename_fmt("MYPATH/RD_%04d.DATA"));
    test_assert_true(rd_valid_basename_fmt("RD_%d.dATA"));
    test_assert_false(rd_valid_basename_fmt("RD_%s.DATA"));
    test_assert_false(rd_valid_basename_fmt("RD_%f.DATA"));
    test_assert_true(rd_valid_basename_fmt("mypath/RD_%d.dATA"));

    exit(0);
}
