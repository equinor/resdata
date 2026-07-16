#include <cstdlib>
#include <ctime>

#include <memory>
#include <iostream>
#include <optional>

#include <resdata/rd_file.hpp>
#include <resdata/rd_util.hpp>

template <typename T>
std::ostream &operator<<(std::ostream &os, const std::optional<T> &opt) {
    if (opt.has_value())
        os << *opt;
    else
        os << "nullopt";
    return os;
}

bool test_get(rd::File *rst_file, int day, int month, int year,
              std::optional<size_t> expected_index) {
    time_t sim_time = rd_make_date(day, month, year);
    auto seqnum_index = rst_file->find_sim_time(sim_time);
    if (seqnum_index == expected_index)
        return true;
    else {
        std::cout << "ERROR:  Date: " << day << "/" << month << "/"
                  << " Got:" << seqnum_index << " Expected:" << expected_index
                  << std::endl;
        return false;
    }
}

int main(int argc, char **argv) {
    bool OK = true;
    const char *unrst_file = argv[1];

    std::unique_ptr<rd::File> rst_file = rd::File::open(unrst_file);

    OK = OK && test_get(rst_file.get(), 1, 1, 1998, std::nullopt);
    OK = OK && test_get(rst_file.get(), 17, 9, 2003, std::nullopt);
    OK = OK && test_get(rst_file.get(), 1, 1, 2008, std::nullopt);

    OK = OK && test_get(rst_file.get(), 1, 1, 2000, 0);
    OK = OK && test_get(rst_file.get(), 1, 10, 2000, 10);
    OK = OK && test_get(rst_file.get(), 1, 3, 2003, 40);
    OK = OK && test_get(rst_file.get(), 31, 12, 2004, 62);

    if (OK)
        exit(0);
    else
        exit(1);
}
