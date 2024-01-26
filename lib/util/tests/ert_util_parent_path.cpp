#include <stdlib.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>

#include <ert/util/util.hpp>
#include <ert/util/test_util.hpp>

void test_path(const char *expected_parent, const char *input_path) {
    char *parent_path = util_alloc_parent_path(input_path);
    test_assert_string_equal(expected_parent, parent_path);
    free(parent_path);
}

int main(int argc, char **argv) {

    test_path("", "path");
    test_path(NULL, "");
    test_path(NULL, NULL);

    test_path("path/parent", "path/parent/leaf");
    test_path("/path/parent", "/path/parent/leaf");
    test_path("/path/parent", "/path/parent/leaf/");
    test_path("/path/parent", "/path/parent/leaf/../leaf");
    test_path("/path", "/path/parent/leaf/..");

    test_path("path/parent", "path/parent/leaf/../leaf");
    test_path("path", "path/parent/leaf/..");

    exit(0);
}
