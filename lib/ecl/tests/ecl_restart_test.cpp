#include <stdlib.h>
#include <stdbool.h>

#include <ert/util/util.h>

#include <ert/ecl/ecl_file.hpp>

bool test_get(ecl_file_type *rst_file, int day, int month, int year,
              int expected_index) {
    time_t sim_time = ecl_util_make_date(day, month, year);
    int seqnum_index = ecl_file_get_restart_index(rst_file, sim_time);
    if (seqnum_index == expected_index)
        return true;
    else {
        printf("ERROR:  Date: %02d/%02d/%4d   Got:%d  Expected:%d \n", day,
               month, year, seqnum_index, expected_index);
        return false;
    }
}

int main(int argc, char **argv) {
    bool OK = true;
    const char *unrst_file = argv[1];

    ecl_file_type *rst_file = ecl_file_open(unrst_file, 0);

    OK = OK && test_get(rst_file, 1, 1, 1998, -1);
    OK = OK && test_get(rst_file, 17, 9, 2003, -1);
    OK = OK && test_get(rst_file, 1, 1, 2008, -1);

    OK = OK && test_get(rst_file, 1, 1, 2000, 0);
    OK = OK && test_get(rst_file, 1, 10, 2000, 10);
    OK = OK && test_get(rst_file, 1, 3, 2003, 40);
    OK = OK && test_get(rst_file, 31, 12, 2004, 62);

    if (OK)
        exit(0);
    else
        exit(1);
}
