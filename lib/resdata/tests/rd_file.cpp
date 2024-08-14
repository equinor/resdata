
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>

#include <ert/util/test_util.hpp>
#include <ert/util/util.hpp>
#include <ert/util/test_work_area.hpp>

#include <resdata/rd_util.hpp>
#include <resdata/rd_file.hpp>
#include <resdata/rd_file_view.hpp>
#include <resdata/rd_grid.hpp>
#include <resdata/rd_endian_flip.hpp>

void test_writable(size_t data_size) {
    rd::util::TestArea ta("file_writable");
    const char *data_file_name = "test_file";

    rd_kw_type *kw = rd_kw_alloc("TEST_KW", data_size, RD_INT);
    for (size_t i = 0; i < data_size; ++i)
        rd_kw_iset_int(kw, i, ((i * 37) + 11) % data_size);

    fortio_type *fortio = fortio_open_writer(data_file_name, false, true);
    rd_kw_fwrite(kw, fortio);
    fortio_fclose(fortio);

    for (int i = 0; i < 4; ++i) {
        rd_file_type *rd_file = rd_file_open(data_file_name, RD_FILE_WRITABLE);
        rd_kw_type *loaded_kw =
            rd_file_view_iget_kw(rd_file_get_global_view(rd_file), 0);
        test_assert_true(rd_kw_equal(kw, loaded_kw));

        rd_file_save_kw(rd_file, loaded_kw);
        rd_file_close(rd_file);
    }

    rd_kw_free(kw);
}

void test_truncated() {
    rd::util::TestArea ta("truncate_file");
    int num_kw;
    {
        rd_grid_type *grid =
            rd_grid_alloc_rectangular(20, 20, 20, 1, 1, 1, NULL);
        rd_grid_fwrite_EGRID2(grid, "TEST.EGRID", RD_METRIC_UNITS);
        rd_grid_free(grid);
    }
    {
        rd_file_type *rd_file = rd_file_open("TEST.EGRID", 0);
        test_assert_true(rd_file_is_instance(rd_file));
        num_kw = rd_file_get_size(rd_file);
        rd_file_close(rd_file);
    }

    {
        offset_type file_size = util_file_size("TEST.EGRID");
        FILE *stream = util_fopen("TEST.EGRID", "r+");
        util_ftruncate(stream, file_size / 2);
        fclose(stream);
    }
    {
        rd_file_type *rd_file = rd_file_open("TEST.EGRID", 0);
        test_assert_true(rd_file_get_size(rd_file) < num_kw);
        rd_file_close(rd_file);
    }
}

void test_mixed_case() {
    rd::util::TestArea ta("mixed_case_file");
    int num_kw;
    {
        rd_grid_type *grid =
            rd_grid_alloc_rectangular(20, 20, 20, 1, 1, 1, NULL);
        rd_grid_fwrite_EGRID2(grid, "TESTcase.EGRID", RD_METRIC_UNITS);
        rd_grid_free(grid);
    }
    {
        rd_file_type *rd_file = rd_file_open("TESTcase.EGRID", 0);
        test_assert_true(rd_file_is_instance(rd_file));
        num_kw = rd_file_get_size(rd_file);
        rd_file_close(rd_file);
    }

    {
        offset_type file_size = util_file_size("TESTcase.EGRID");
        FILE *stream = util_fopen("TESTcase.EGRID", "r+");
        util_ftruncate(stream, file_size / 2);
        fclose(stream);
    }
    {
        rd_file_type *rd_file = rd_file_open("TESTcase.EGRID", 0);
        test_assert_true(rd_file_get_size(rd_file) < num_kw);
        rd_file_close(rd_file);
    }
}

int main(int argc, char **argv) {
    test_writable(10);
    test_writable(1337);
    test_truncated();
    test_mixed_case();
    exit(0);
}
