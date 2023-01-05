#include <stdlib.h>
#include <stdbool.h>

#include <ert/util/test_util.hpp>
#include <ert/util/stringlist.hpp>
#include <ert/util/util.h>

int main(int argc, char **argv) {
    test_install_SIGNALS();
    test_assert_true(util_ping("localhost"));
    test_assert_true(util_ping("127.0.0.1"));
    test_assert_false(util_ping("does.not.exist"));

    exit(0);
}
