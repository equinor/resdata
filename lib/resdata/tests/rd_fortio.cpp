#include <cstdio>
#include <cstdlib>

#include <exception>
#include <ios>
#include <string>

#include <ert/util/test_util.hpp>
#include <ert/util/util.hpp>
#include <ert/util/vector.hpp>
#include <ert/util/test_work_area.hpp>

#include <resdata/FortIO.hpp>
#include <resdata/rd_endian_flip.hpp>

void test_existing_read(const char *filename) {
    // should not raise
    ERT::FortIO fortio(filename, std::ios_base::in);
}

void test_not_existing_read() {
    test_assert_throw(ERT::FortIO("/does/not/exist", std::ios_base::in),
                      std::ios_base::failure);
}

void test_write(const char *filename, bool path_exists) {
    bool raised = false;
    try {
        ERT::FortIO fortio(filename, std::ios_base::out);
    } catch (const std::exception &) {
        raised = true;
    }
    test_assert_true(raised == !path_exists);
}

void test_wrapper(const char *filename) {
    FILE *stream = util_fopen(filename, "r");
    ERT::FortIO fortio(filename, false, false, stream, false);

    test_assert_false(fortio.fclose_stream());
    test_assert_false(fortio.fopen_stream());
    test_assert_true(fortio.stream_is_open());

    fclose(stream);
}

void test_open_close_read(const char *filename) {
    ERT::FortIO fortio(filename, std::ios_base::in);

    test_assert_true(fortio.stream_is_open());
    test_assert_true(fortio.fclose_stream());
    test_assert_false(fortio.stream_is_open());
    test_assert_false(fortio.fclose_stream());
    test_assert_true(fortio.fopen_stream());
    test_assert_true(fortio.stream_is_open());
    test_assert_false(fortio.fopen_stream());
}

void test_fread_truncated_data() {
    rd::util::TestArea work_area("fortio_truncated");
    {
        const size_t buffer_size = 1000;
        char *buffer = (char *)util_malloc(buffer_size);
        {
            ERT::FortIO fortio("PRESSURE", std::ios_base::out, false, true);

            fortio.fwrite_record(buffer, buffer_size);
            fortio.fwrite_record(buffer, buffer_size);

            fortio.fseek(0, SEEK_SET);
            util_ftruncate(fortio.get_FILE(), 2 * buffer_size - 100);
        }

        test_assert_long_equal(util_file_size("PRESSURE"),
                               2 * buffer_size - 100);

        {
            ERT::FortIO fortio("PRESSURE", std::ios_base::in, false, true);
            test_assert_true(fortio.fread_buffer(buffer, buffer_size));
            test_assert_false(fortio.fread_buffer(buffer, buffer_size));
        }
        free(buffer);
    }
}

void test_fread_truncated_head() {
    rd::util::TestArea work_area("fortio_truncated");
    {
        {
            FILE *stream = util_fopen("PRESSURE", "w");
            fclose(stream);
        }

        {
            ERT::FortIO fortio("PRESSURE", std::ios_base::in, false, true);
            char *buffer = NULL;
            int buffer_size = 10;
            test_assert_false(fortio.fread_buffer(buffer, buffer_size));
            test_assert_true(fortio.read_at_eof());
        }
    }
}

void test_fread_truncated_tail() {
    rd::util::TestArea work_area("fortio_truncated3");
    {
        const size_t buffer_size = 1000;
        char *buffer = (char *)util_malloc(buffer_size);
        {
            ERT::FortIO fortio("PRESSURE", std::ios_base::out, false, true);

            fortio.fwrite_record(buffer, buffer_size);
            fortio.fseek(0, SEEK_SET);
            util_ftruncate(fortio.get_FILE(), buffer_size + 4);
        }

        test_assert_long_equal(util_file_size("PRESSURE"), buffer_size + 4);

        {
            ERT::FortIO fortio("PRESSURE", std::ios_base::in, false, true);
            test_assert_false(fortio.fread_buffer(buffer, buffer_size));
        }
        free(buffer);
    }
}

void test_fread_invalid_tail() {
    rd::util::TestArea work_area("fortio_invalid");
    int record_size = 10;
    char *buffer = (char *)util_malloc(record_size);
    {
        FILE *stream = util_fopen("PRESSURE", "w");

        util_fwrite(&record_size, sizeof record_size, 1, stream, __func__);
        util_fwrite(buffer, 1, record_size, stream, __func__);
        util_fwrite(&record_size, sizeof record_size, 1, stream, __func__);

        util_fwrite(&record_size, sizeof record_size, 1, stream, __func__);
        util_fwrite(buffer, 1, record_size, stream, __func__);
        record_size += 1;
        util_fwrite(&record_size, sizeof record_size, 1, stream, __func__);

        fclose(stream);
    }
    {
        ERT::FortIO fortio("PRESSURE", std::ios_base::in, false, false);
        record_size -= 1;
        test_assert_true(fortio.fread_buffer(buffer, record_size));
        test_assert_false(fortio.fread_buffer(buffer, record_size));
    }

    free(buffer);
}

void test_at_eof() {
    rd::util::TestArea work_area("fortio_truncated3");
    {
        ERT::FortIO fortio("PRESSURE", std::ios_base::out, false, true);
        char *buffer = (char *)util_malloc(100);

        fortio.fwrite_record(buffer, 100);
        free(buffer);
    }
    {
        ERT::FortIO fortio("PRESSURE", std::ios_base::in, false, true);

        test_assert_false(fortio.read_at_eof());
        fortio.fseek(50, SEEK_SET);
        test_assert_false(fortio.read_at_eof());
        fortio.fseek(0, SEEK_END);
        test_assert_true(fortio.read_at_eof());
    }
}

void test_fseek() {
    rd::util::TestArea work_area("fortio_fseek");
    {
        ERT::FortIO fortio("PRESSURE", std::ios_base::out, false, true);
        char *buffer = (char *)util_malloc(100);

        fortio.fwrite_record(buffer, 100);
        free(buffer);
    }
    {
        ERT::FortIO fortio("PRESSURE", std::ios_base::in, false, true);

        test_assert_true(fortio.fseek(0, SEEK_SET));
        test_assert_true(fortio.fseek(0, SEEK_END));
        test_assert_false(fortio.fseek(100000, SEEK_END));
        test_assert_false(fortio.fseek(100000, SEEK_SET));
    }
}

void test_write_failure() {
    rd::util::TestArea work_area("fortio_truncated");
    {
        ERT::FortIO fortio("PRESSURE", std::ios_base::out, false, true);
        char *buffer = (char *)util_malloc(100);

        fortio.fwrite_record(buffer, 100);
        test_assert_true(util_file_exists("PRESSURE"));
        fortio.fwrite_error();
        test_assert_false(util_file_exists("PRESSURE"));
        fortio.fwrite_record(buffer, 100);
        free(buffer);
        test_assert_false(util_file_exists("PRESSURE"));
    }
}

int main(int argc, char **argv) {
    util_install_signals();
    {
        const char *file = argv[1];

        test_existing_read(file);
        test_not_existing_read();
        test_open_close_read(file);
        test_wrapper(file);
        test_fread_truncated_head();
        test_fread_truncated_data();
        test_fread_truncated_tail();
        test_fread_invalid_tail();
        test_fseek();
        test_at_eof();

        test_write("/tmp/path/does/not/exist", false);
        {
            rd::util::TestArea work_area("rd_fortio_write");
            util_make_path("path");
            test_write("path/file.x", true);
        }

        test_write_failure();

        exit(0);
    }
}
