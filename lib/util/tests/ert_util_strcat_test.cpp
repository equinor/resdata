#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include <ert/util/vector.hpp>
#include <ert/util/util.hpp>
#include <ert/util/test_util.hpp>

void test_strcat(char *s1, const char *s2, const char *expected) {
    char *cat = util_strcat_realloc(s1, s2);
    if (test_check_string_equal(cat, expected))
        free(cat);
    else {
        fprintf(stderr, "util_strcat_realloc(%s,%s) Got:%s  expected:%s \n", s1,
                s2, cat, expected);
        free(cat);
        exit(1);
    }
}

int main(int argc, char **argv) {
    test_strcat(NULL, NULL, NULL);

    {
        const char *s = "Hei";
        char *c = util_alloc_string_copy(s);
        test_strcat(NULL, c, s);
        free(c);
    }
    {
        const char *s = "Hei";
        test_strcat(util_alloc_string_copy(s), NULL, s);
    }
    {
        char *s1 = util_alloc_string_copy("hei");
        char *s2 = util_alloc_string_copy("-Hei");
        test_strcat(s1, s2, "hei-Hei");
        free(s2);
    }

    printf("Test OK\n");
    exit(0);
}
