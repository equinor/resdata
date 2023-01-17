#include <stdlib.h>
#include <stdbool.h>

#include <ert/util/vector.hpp>
#include <ert/util/util.h>
#include <ert/util/test_util.hpp>

int main(int argc, char **argv) {
    char *cwd = argv[1];
    char *cwd_alloc = util_alloc_cwd();
    printf("cwd    :%s\n", cwd_alloc);
    printf("argv[1]:%s\n", argv[1]);

    if (!util_is_cwd(cwd_alloc))
        test_error_exit("Hmmm did not recognize:%s as cwd\n", cwd);

    if (util_is_cwd("/some/path"))
        test_error_exit("Took /whatver/ as CWD\n");

    exit(0);
}
