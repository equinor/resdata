#include <cstdio>
#include <cstdlib>

#include <ios>
#include <string>
#include <filesystem>
#include <utility>
#include <vector>

#include <ert/util/test_util.hpp>
#include <ert/util/util.hpp>
#include <ert/util/test_work_area.hpp>

#include <resdata/rd_util.hpp>
#include <resdata/rd_file.hpp>
#include <resdata/rd_file_view.hpp>
#include <resdata/rd_grid.hpp>
#include <resdata/FortIO.hpp>
#include <resdata/rd_kw.hpp>
#include <resdata/rd_type.hpp>
#include <resdata/rd_file_flag.hpp>

void test_writable(size_t data_size) {
    rd::util::TestArea ta("file_writable");
    const char *data_file_name = "test_file";

    std::vector<int> data(data_size);
    for (size_t i = 0; i < data_size; ++i)
        data[i] = ((i * 37) + 11) % data_size;
    rd::KW kw{"TEST_KW", std::move(data)};

    {
        ERT::FortIO fortio(data_file_name, std::ios_base::out, false, true);
        kw.fwrite(fortio);
    }

    for (int i = 0; i < 4; ++i) {
        auto rd_file = rd::File::open(data_file_name, FileMode::WRITABLE);
        rd::KW *loaded_kw = rd_file->get_global_view()->get_kw(0);
        test_assert_true(kw == *loaded_kw);

        rd_file->save_kw(loaded_kw);
    }
}

void test_truncated() {
    rd::util::TestArea ta("truncate_file");
    size_t num_kw{};
    {
        rd_grid_ptr grid = make_rectangular_grid(20, 20, 20, 1, 1, 1, NULL);
        rd_grid_fwrite_EGRID2(grid.get(), "TEST.EGRID", UnitSystem::METRIC);
    }
    {
        auto rd_file = rd::File::open("TEST.EGRID");
        num_kw = rd_file->size();
    }

    {
        auto file_size = std::filesystem::file_size("TEST.EGRID");
        std::filesystem::resize_file("TEST.EGRID", file_size / 2);
    }

    {
        auto rd_file = rd::File::open("TEST.EGRID");
        test_assert_true(rd_file->size() < num_kw);
    }
}

void test_mixed_case() {
    rd::util::TestArea ta("mixed_case_file");
    size_t num_kw{};
    {
        rd_grid_ptr grid = make_rectangular_grid(20, 20, 20, 1, 1, 1, NULL);
        rd_grid_fwrite_EGRID2(grid.get(), "TESTcase.EGRID", UnitSystem::METRIC);
    }
    {
        auto rd_file = rd::File::open("TESTcase.EGRID");
        num_kw = rd_file->size();
    }

    {
        auto file_size = std::filesystem::file_size("TESTcase.EGRID");
        std::filesystem::resize_file("TESTcase.EGRID", file_size / 2);
    }

    {
        auto rd_file = rd::File::open("TESTcase.EGRID");
        test_assert_true(rd_file->size() < num_kw);
    }
}

int main(int argc, char **argv) {
    test_writable(10);
    test_writable(1337);
    test_truncated();
    test_mixed_case();
    exit(0);
}
