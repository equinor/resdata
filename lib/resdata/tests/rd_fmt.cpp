#include <cstdlib>
#include <filesystem>
#include <string>

#include <ert/util/util.hpp>
#include <ert/util/test_work_area.hpp>
#include <ert/util/test_util.hpp>

#include <resdata/rd_util.hpp>

void test_content(const rd::util::TestArea &ta, const char *src_file,
                  bool fmt_file) {
    ta.copy_file(src_file);
    {
        const std::filesystem::path src_path = src_file;
        const std::string file_name = src_path.filename().string();
        const std::string base_name = src_path.stem().string();
        bool fmt;

        std::filesystem::copy_file(
            file_name, base_name,
            std::filesystem::copy_options::overwrite_existing);

        test_assert_true(rd_fmt_file(base_name.c_str(), &fmt));
        test_assert_bool_equal(fmt, fmt_file);
    }
}

void test_small() {
    bool fmt;

    FILE *stream = util_fopen("small.txt", "w");
    fprintf(stream, "Some bytes\n");
    fclose(stream);

    test_assert_false(rd_fmt_file("small.txt", &fmt));
}

int main(int argc, char **argv) {
    rd::util::TestArea ta("rd_fmt");
    {
        const char *binary_file = argv[1];
        const char *text_file = argv[2];

        bool fmt_file;

        test_assert_true(rd_fmt_file(binary_file, &fmt_file));
        test_assert_false(fmt_file);

        test_assert_true(rd_fmt_file(text_file, &fmt_file));
        test_assert_true(fmt_file);

        test_assert_true(rd_fmt_file("TEST.EGRID", &fmt_file));
        test_assert_false(fmt_file);

        test_assert_true(rd_fmt_file("TEST.FEGRID", &fmt_file));
        test_assert_true(fmt_file);

        test_assert_false(rd_fmt_file("TEST_DOES_NOT_EXIST", &fmt_file));

        test_content(ta, binary_file, false);
        test_content(ta, text_file, true);
        test_small();
    }
    exit(0);
}
