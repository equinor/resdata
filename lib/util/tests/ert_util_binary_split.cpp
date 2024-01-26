#include <stdlib.h>
#include <stdbool.h>

#include <ert/util/test_util.hpp>
#include <ert/util/util.hpp>

void test_split(const char *test_string, bool split_on_first, const char *true1,
                const char *true2) {
    char *part1;
    char *part2;

    util_binary_split_string(test_string, ":", split_on_first, &part1, &part2);
    test_assert_string_equal(true1, part1);
    test_assert_string_equal(true2, part2);
    free(part1);
    free(part2);

    util_binary_split_string(test_string, ":;", split_on_first, &part1, &part2);
    test_assert_string_equal(true1, part1);
    test_assert_string_equal(true2, part2);
    free(part1);
    free(part2);

    util_binary_split_string(test_string, ";", split_on_first, &part1, &part2);
    test_assert_string_equal(test_string, part1);
    test_assert_string_equal(NULL, part2);
    free(part1);
}

int main(int argc, char **argv) {

    test_split("Hello:Hello", true, "Hello", "Hello");
    test_split("ABC:DEF:GEH", true, "ABC", "DEF:GEH");
    test_split("ABC:DEF:GEH", false, "ABC:DEF", "GEH");
    test_split("ABC:DEF:GEH:", false, "ABC:DEF", "GEH");
    test_split("ABC:DEF:GEH:", true, "ABC", "DEF:GEH");
    test_split("ABCDEFGEH", false, "ABCDEFGEH", NULL);
    test_split("ABCDEFGEH:", false, "ABCDEFGEH", NULL);
    test_split(":ABCDEFGEH", false, "ABCDEFGEH", NULL);
    test_split(NULL, false, NULL, NULL);

    exit(0);
}
