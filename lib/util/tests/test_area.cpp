#include <string>

#include <ert/util/test_util.hpp>
#include <ert/util/test_work_area.hpp>

void test_create() {
    ecl::util::TestArea ta("Name");

    test_assert_true(ta.test_cwd() != ta.original_cwd());
}

int main(int argc, char **argv) { test_create(); }
