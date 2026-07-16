#include <cstdio>
#include <cstdlib>

#include <ios>
#include <string>

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

    rd_kw_type *kw = rd_kw_alloc("TEST_KW", data_size, RD_INT);
    for (size_t i = 0; i < data_size; ++i)
        rd_kw_iset_int(kw, i, ((i * 37) + 11) % data_size);

    {
        ERT::FortIO fortio(data_file_name, std::ios_base::out, false, true);
        rd_kw_fwrite(kw, fortio);
    }

    for (int i = 0; i < 4; ++i) {
        auto rd_file = rd::File::open(data_file_name, FileMode::WRITABLE);
        rd_kw_type *loaded_kw =
            rd_file_get_global_view(rd_file.get())->get_kw(0);
        test_assert_true(rd_kw_equal(kw, loaded_kw));

        rd_file_save_kw(rd_file.get(), loaded_kw);
    }

    rd_kw_free(kw);
}

void test_truncated() {
    rd::util::TestArea ta("truncate_file");
    size_t num_kw{};
    {
        rd_grid_ptr grid = make_rectangular_grid(20, 20, 20, 1, 1, 1, NULL);
        rd_grid_fwrite_EGRID2(grid.get(), "TEST.EGRID", RD_METRIC_UNITS);
    }
    {
        auto rd_file = rd::File::open("TEST.EGRID");
        num_kw = rd_file->size();
    }

    {
        offset_type file_size = util_file_size("TEST.EGRID");
        FILE *stream = util_fopen("TEST.EGRID", "r+");
        util_ftruncate(stream, file_size / 2);
        fclose(stream);
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
        rd_grid_fwrite_EGRID2(grid.get(), "TESTcase.EGRID", RD_METRIC_UNITS);
    }
    {
        auto rd_file = rd::File::open("TESTcase.EGRID");
        num_kw = rd_file->size();
    }

    {
        offset_type file_size = util_file_size("TESTcase.EGRID");
        FILE *stream = util_fopen("TESTcase.EGRID", "r+");
        util_ftruncate(stream, file_size / 2);
        fclose(stream);
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
