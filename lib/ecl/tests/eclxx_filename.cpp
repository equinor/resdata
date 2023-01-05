#include <ert/util/test_util.hpp>
#include <ert/ecl/EclFilename.hpp>

void cmp(const char *expected, const std::string &value) {
    test_assert_string_equal(expected, value.c_str());
}

int main(int argc, char **argv) {
    cmp("BASE.X0067", ERT::EclFilename("BASE", ECL_RESTART_FILE, 67));
    cmp("BASE.F0067", ERT::EclFilename("BASE", ECL_RESTART_FILE, 67, true));

    cmp("BASE.EGRID", ERT::EclFilename("BASE", ECL_EGRID_FILE));
    cmp("BASE.FEGRID", ERT::EclFilename("BASE", ECL_EGRID_FILE, true));

    cmp("BASE.EGRID", ERT::EclFilename("BASE", ECL_EGRID_FILE, 67));
    cmp("BASE.FEGRID", ERT::EclFilename("BASE", ECL_EGRID_FILE, 67, true));

    try {
        ERT::EclFilename("BASE", ECL_RESTART_FILE);
        test_assert_true(false);
    } catch (...) {
        test_assert_true(true);
    }

    cmp("PATH/BASE.X0067",
        ERT::EclFilename("PATH", "BASE", ECL_RESTART_FILE, 67));
    cmp("PATH/BASE.F0067",
        ERT::EclFilename("PATH", "BASE", ECL_RESTART_FILE, 67, true));

    cmp("PATH/BASE.EGRID", ERT::EclFilename("PATH", "BASE", ECL_EGRID_FILE));
    cmp("PATH/BASE.FEGRID",
        ERT::EclFilename("PATH", "BASE", ECL_EGRID_FILE, true));

    cmp("PATH/BASE.EGRID",
        ERT::EclFilename("PATH", "BASE", ECL_EGRID_FILE, 67));
    cmp("PATH/BASE.FEGRID",
        ERT::EclFilename("PATH", "BASE", ECL_EGRID_FILE, 67, true));

    try {
        ERT::EclFilename("PATH", "BASE", ECL_RESTART_FILE);
        test_assert_true(false);
    } catch (...) {
        test_assert_true(true);
    }

    test_assert_int_equal(ECL_EGRID_FILE, ERT::EclFiletype("CASE.EGRID"));
    test_assert_int_equal(ECL_RESTART_FILE, ERT::EclFiletype("CASE.F0098"));
    test_assert_int_equal(ECL_UNIFIED_RESTART_FILE,
                          ERT::EclFiletype("CASE.UNRST"));
}
