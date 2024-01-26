#include <stdlib.h>
#include <stdbool.h>

#include <ert/util/test_util.hpp>
#include <ert/util/util.hpp>
#include <ert/util/test_work_area.hpp>

#include <resdata/rd_endian_flip.hpp>
#include <resdata/rd_kw.hpp>
#include <resdata/fortio.h>
#include <resdata/rd_file.hpp>
#include <resdata/rd_file_view.hpp>

void test_truncated(const char *filename, offset_type truncate_size) {
    {
        FILE *stream = util_fopen(filename, "r+");
        util_ftruncate(stream, truncate_size);
        fclose(stream);
    }
    {
        fortio_type *fortio = fortio_open_reader(filename, false, true);
        rd_kw_type *kw2 = rd_kw_fread_alloc(fortio);
        test_assert_NULL(kw2);
        fortio_fclose(fortio);
    }
}

void test_fread_alloc() {
    rd::util::TestArea ta("fread_alloc");
    {
        rd_kw_type *kw1 = rd_kw_alloc("INT", 100, RD_INT);
        int i;
        for (i = 0; i < 100; i++)
            rd_kw_iset_int(kw1, i, i);
        {
            fortio_type *fortio = fortio_open_writer("INT", false, true);
            rd_kw_fwrite(kw1, fortio);
            fortio_fclose(fortio);
        }
        {
            fortio_type *fortio = fortio_open_reader("INT", false, true);
            rd_kw_type *kw2 = rd_kw_fread_alloc(fortio);
            test_assert_true(rd_kw_is_instance(kw2));
            test_assert_true(rd_kw_equal(kw1, kw2));
            rd_kw_free(kw2);
            fortio_fclose(fortio);
        }

        {
            offset_type file_size = util_file_size("INT");
            test_truncated("INT", file_size - 4);
            test_truncated("INT", file_size - 25);
            test_truncated("INT", 5);
            test_truncated("INT", 0);
        }
        rd_kw_free(kw1);
    }
}

void test_kw_io_charlength() {
    rd::util::TestArea ta("io_charlength");
    {
        const char *KW0 = "QWERTYUI";
        const char *KW1 = "ABCDEFGHIJTTTTTTTTTTTTTTTTTTTTTTABCDEFGHIJKLMNOP";
        rd_kw_type *rd_kw_out0 = rd_kw_alloc(KW0, 5, RD_FLOAT);
        rd_kw_type *rd_kw_out1 = rd_kw_alloc(KW1, 5, RD_FLOAT);
        for (int i = 0; i < rd_kw_get_size(rd_kw_out1); i++) {
            rd_kw_iset_float(rd_kw_out0, i, i * 1.5);
            rd_kw_iset_float(rd_kw_out1, i, i * 1.5);
        }

        {
            fortio_type *f = fortio_open_writer("TEST1", false, RD_ENDIAN_FLIP);
            test_assert_true(rd_kw_fwrite(rd_kw_out0, f));
            test_assert_false(rd_kw_fwrite(rd_kw_out1, f));
            fortio_fclose(f);
        }

        { test_assert_false(util_file_exists("TEST1")); }

        {
            FILE *file = util_fopen("TEST2", "w");
            rd_kw_fprintf_grdecl(rd_kw_out1, file);
            fclose(file);
        }

        {
            FILE *file = util_fopen("TEST2", "r");
            rd_kw_type *rd_kw_in =
                rd_kw_fscanf_alloc_grdecl(file, KW1, -1, RD_FLOAT);
            test_assert_string_equal(KW1, rd_kw_get_header(rd_kw_in));
            test_assert_int_equal(5, rd_kw_get_size(rd_kw_in));

            test_assert_double_equal(rd_kw_iget_as_double(rd_kw_in, 0), 0.0);
            test_assert_double_equal(rd_kw_iget_as_double(rd_kw_in, 4), 6.0);

            rd_kw_free(rd_kw_in);
            fclose(file);
        }

        rd_kw_free(rd_kw_out0);
        rd_kw_free(rd_kw_out1);
    }
}

int main(int argc, char **argv) {
    test_fread_alloc();
    test_kw_io_charlength();
    exit(0);
}
