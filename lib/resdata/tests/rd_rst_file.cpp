#include <stdlib.h>
#include <stdbool.h>

#include <ert/util/test_util.hpp>
#include <ert/util/test_work_area.hpp>

#include <resdata/rd_kw.hpp>
#include <resdata/rd_kw_magic.hpp>
#include <resdata/rd_endian_flip.hpp>
#include <resdata/rd_rst_file.hpp>
#include <resdata/rd_type.hpp>

void write_keyword(fortio_type *fortio, const char *kw,
                   rd_data_type data_type) {
    rd_kw_type *rd_kw = rd_kw_alloc(kw, 1000, data_type);
    rd_kw_fwrite(rd_kw, fortio);
    rd_kw_free(rd_kw);
}

void write_seqnum(fortio_type *fortio, int report_step) {
    rd_kw_type *rd_kw = rd_kw_alloc(SEQNUM_KW, 1, RD_INT);
    rd_kw_iset_int(rd_kw, 0, report_step);
    rd_kw_fwrite(rd_kw, fortio);
    rd_kw_free(rd_kw);
}

void test_empty() {
    rd_rst_file_type *rst_file = rd_rst_file_open_write_seek("EMPTY.UNRST", 0);
    test_assert_int_equal(rd_rst_file_ftell(rst_file), 0);
    rd_rst_file_close(rst_file);
}

void test_file(const char *src_file, const char *target_file, int report_step,
               offset_type expected_offset) {
    util_copy_file(src_file, target_file);
    {
        rd_rst_file_type *rst_file =
            rd_rst_file_open_write_seek(target_file, report_step);
        test_assert_true(rd_rst_file_ftell(rst_file) == expected_offset);
        rd_rst_file_close(rst_file);
        test_assert_true(util_file_size(target_file) ==
                         (size_t)expected_offset);
    }
}

void test_Xfile() {
    rd::util::TestArea ta("xfile");
    {
        fortio_type *f =
            fortio_open_writer("TEST.X0010", false, RD_ENDIAN_FLIP);

        write_keyword(f, "INTEHEAD", RD_INT);
        write_keyword(f, "PRESSURE", RD_FLOAT);
        write_keyword(f, "SWAT", RD_FLOAT);

        write_keyword(f, "INTEHEAD", RD_INT);
        write_keyword(f, "PRESSURE", RD_FLOAT);
        write_keyword(f, "SWAT", RD_FLOAT);

        fortio_fclose(f);
    }
    test_file("TEST.X0010", "FILE.X0010", 10, 0);
}

void test_UNRST0() {
    rd::util::TestArea ta("rst-file");
    offset_type pos10;
    offset_type pos20;
    offset_type pos_end;
    {
        fortio_type *f =
            fortio_open_writer("TEST.UNRST", false, RD_ENDIAN_FLIP);
        write_seqnum(f, 0);
        write_keyword(f, "INTEHEAD", RD_INT);
        write_keyword(f, "PRESSURE", RD_FLOAT);
        write_keyword(f, "SWAT", RD_FLOAT);

        pos10 = fortio_ftell(f);
        write_seqnum(f, 10);
        write_keyword(f, "INTEHEAD", RD_INT);
        write_keyword(f, "PRESSURE", RD_FLOAT);
        write_keyword(f, "SWAT", RD_FLOAT);

        pos20 = fortio_ftell(f);
        write_seqnum(f, 20);
        write_keyword(f, "INTEHEAD", RD_INT);
        write_keyword(f, "PRESSURE", RD_FLOAT);
        write_keyword(f, "SWAT", RD_FLOAT);

        pos_end = fortio_ftell(f);
        fortio_fclose(f);
    }
    test_file("TEST.UNRST", "FILE.UNRST", 0, 0);
    test_file("TEST.UNRST", "FILE.UNRST", 5, pos10);
    test_file("TEST.UNRST", "FILE.UNRST", 10, pos10);
    test_file("TEST.UNRST", "FILE.UNRST", 15, pos20);
    test_file("TEST.UNRST", "FILE.UNRST", 20, pos20);
    test_file("TEST.UNRST", "FILE.UNRST", 25, pos_end);
}

void test_UNRST1() {
    rd::util::TestArea ta("rst-file");
    offset_type pos5;
    offset_type pos10;
    offset_type pos20;
    offset_type pos_end;
    {
        fortio_type *f =
            fortio_open_writer("TEST.UNRST", false, RD_ENDIAN_FLIP);
        pos5 = fortio_ftell(f);
        write_seqnum(f, 5);
        write_keyword(f, "INTEHEAD", RD_INT);
        write_keyword(f, "PRESSURE", RD_FLOAT);
        write_keyword(f, "SWAT", RD_FLOAT);

        pos10 = fortio_ftell(f);
        write_seqnum(f, 10);
        write_keyword(f, "INTEHEAD", RD_INT);
        write_keyword(f, "PRESSURE", RD_FLOAT);
        write_keyword(f, "SWAT", RD_FLOAT);

        pos20 = fortio_ftell(f);
        write_seqnum(f, 20);
        write_keyword(f, "INTEHEAD", RD_INT);
        write_keyword(f, "PRESSURE", RD_FLOAT);
        write_keyword(f, "SWAT", RD_FLOAT);

        pos_end = fortio_ftell(f);
        fortio_fclose(f);
    }
    test_file("TEST.UNRST", "FILE.UNRST", 0, 0);
    test_file("TEST.UNRST", "FILE.UNRST", 1, 0);
    test_file("TEST.UNRST", "FILE.UNRST", 5, pos5);
    test_file("TEST.UNRST", "FILE.UNRST", 10, pos10);
    test_file("TEST.UNRST", "FILE.UNRST", 15, pos20);
    test_file("TEST.UNRST", "FILE.UNRST", 20, pos20);
    test_file("TEST.UNRST", "FILE.UNRST", 25, pos_end);
}

int main(int argc, char **argv) {
    test_empty();
    test_Xfile();
    test_UNRST0();
    test_UNRST1();
}
