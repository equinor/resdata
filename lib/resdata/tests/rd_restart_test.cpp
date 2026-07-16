#include <cstdlib>

#include <ert/util/util.hpp>

#include <resdata/rd_file.hpp>

bool test_get(rd_file_type *rst_file, int day, int month, int year,
              int expected_index) {
    time_t sim_time = rd_make_date(day, month, year);
    int seqnum_index = rst_file->find_sim_time(sim_time);
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

    rd_file_ptr rst_file = rd::File::open(unrst_file);

    OK = OK && test_get(rst_file.get(), 1, 1, 1998, -1);
    OK = OK && test_get(rst_file.get(), 17, 9, 2003, -1);
    OK = OK && test_get(rst_file.get(), 1, 1, 2008, -1);

    OK = OK && test_get(rst_file.get(), 1, 1, 2000, 0);
    OK = OK && test_get(rst_file.get(), 1, 10, 2000, 10);
    OK = OK && test_get(rst_file.get(), 1, 3, 2003, 40);
    OK = OK && test_get(rst_file.get(), 31, 12, 2004, 62);

    if (OK)
        exit(0);
    else
        exit(1);
}
