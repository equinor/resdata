#include <ert/util/test_util.hpp>
#include <resdata/RDFilename.hpp>

void cmp(const char *expected, const std::string &value) {
    test_assert_string_equal(expected, value.c_str());
}

int main(int argc, char **argv) {
    cmp("BASE.X0067", ERT::RDFilename("BASE", RD_RESTART_FILE, 67));
    cmp("BASE.F0067", ERT::RDFilename("BASE", RD_RESTART_FILE, 67, true));

    cmp("BASE.EGRID", ERT::RDFilename("BASE", RD_EGRID_FILE));
    cmp("BASE.FEGRID", ERT::RDFilename("BASE", RD_EGRID_FILE, true));

    cmp("BASE.EGRID", ERT::RDFilename("BASE", RD_EGRID_FILE, 67));
    cmp("BASE.FEGRID", ERT::RDFilename("BASE", RD_EGRID_FILE, 67, true));

    try {
        ERT::RDFilename("BASE", RD_RESTART_FILE);
        test_assert_true(false);
    } catch (...) {
        test_assert_true(true);
    }

    cmp("PATH/BASE.X0067",
        ERT::RDFilename("PATH", "BASE", RD_RESTART_FILE, 67));
    cmp("PATH/BASE.F0067",
        ERT::RDFilename("PATH", "BASE", RD_RESTART_FILE, 67, true));

    cmp("PATH/BASE.EGRID", ERT::RDFilename("PATH", "BASE", RD_EGRID_FILE));
    cmp("PATH/BASE.FEGRID",
        ERT::RDFilename("PATH", "BASE", RD_EGRID_FILE, true));

    cmp("PATH/BASE.EGRID", ERT::RDFilename("PATH", "BASE", RD_EGRID_FILE, 67));
    cmp("PATH/BASE.FEGRID",
        ERT::RDFilename("PATH", "BASE", RD_EGRID_FILE, 67, true));

    try {
        ERT::RDFilename("PATH", "BASE", RD_RESTART_FILE);
        test_assert_true(false);
    } catch (...) {
        test_assert_true(true);
    }

    test_assert_int_equal(RD_EGRID_FILE, ERT::RDFiletype("CASE.EGRID"));
    test_assert_int_equal(RD_RESTART_FILE, ERT::RDFiletype("CASE.F0098"));
    test_assert_int_equal(RD_UNIFIED_RESTART_FILE,
                          ERT::RDFiletype("CASE.UNRST"));
}
