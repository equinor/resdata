#include <stdlib.h>
#include <stdbool.h>

#include <ert/util/util.hpp>
#include <ert/util/test_util.hpp>
#include <ert/util/stringlist.hpp>

void test_path(const char *input, const char *expected) {
    char *rpath = util_alloc_realpath__(input);
    if (!test_check_string_equal(rpath, expected))
        test_error_exit("util_alloc_realpath__(%s) => %s  expected:%s \n",
                        input, rpath, expected);
    else
        printf("OK: %s -> %s \n", input, rpath);

    free(rpath);
}

int main(int argc, char **argv) {
#ifdef ERT_LINUX

    test_path("/tmp/", "/tmp");
    test_path("/tmp/test/normal", "/tmp/test/normal");
    test_path("/tmp/test/../test/normal", "/tmp/test/normal");
    test_path("/tmp/test/../../tmp/test/normal", "/tmp/test/normal");
    test_path("/tmp/test/../../tmp//test/normal", "/tmp/test/normal");
    test_path("/tmp/test/../../tmp/./test/normal", "/tmp/test/normal");
    test_path("/tmp/test/../../tmp/./test/normal/", "/tmp/test/normal");
    test_path("/tmp/test/../../tmp/other/XX/", "/tmp/other/XX");

#endif

    exit(0);
}
