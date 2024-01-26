#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <ert/util/util.hpp>
#include <ert/util/test_util.hpp>

void assert_equal(bool equal) {
    if (!equal)
        exit(1);
}

int main(int argc, char **argv) {
    test_assert_true(util_file_readable(argv[0]));
    {
        char *path;
        util_alloc_file_components(argv[0], &path, NULL, NULL);
        test_assert_false(util_file_readable(path));
        free(path);
    }
    {
        const char *file = "/tmp/test_file.txt";
        mode_t mode = 0;
        FILE *stream = util_fopen(file, "w");
        fclose(stream);

        chmod(file, mode);
        test_assert_false(util_file_readable(file));
        unlink(file);
    }
    exit(0);
}
