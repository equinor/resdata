#include <cstdio>
#include <cstdlib>

#include <filesystem>
#include <string>

#include <ert/util/test_work_area.hpp>
#include <ert/util/test_util.hpp>
#include <ert/util/stringlist.hpp>
#include <ert/util/util.hpp>

#include <resdata/rd_util.hpp>

void test_filename_report_nr() {
    test_assert_int_equal(
        78, rd_filename_report_nr("Path/with/mixedCASE/case.x0078"));
    test_assert_int_equal(78, rd_filename_report_nr("Case.X0078"));
    test_assert_true(
        FileType::EGRID ==
        rd_get_file_type("path/WITH/xase/MyGrid.EGrid", NULL, NULL));
}

void test_filename_case() {
    std::string f1 =
        rd::filename("mixedBase", FileType::EGRID, false, -1).string();
    std::string f2 = rd::filename("UPPER", FileType::EGRID, false, -1).string();
    std::string f3 = rd::filename("lower", FileType::EGRID, false, -1).string();
    std::string f4 =
        rd::filename("lower1", FileType::EGRID, false, -1).string();
    std::string f5 =
        rd::filename("UPPER1", FileType::EGRID, false, -1).string();

    test_assert_string_equal(f1.c_str(), "mixedBase.EGRID");
    test_assert_string_equal(f2.c_str(), "UPPER.EGRID");
    test_assert_string_equal(f3.c_str(), "lower.egrid");
    test_assert_string_equal(f4.c_str(), "lower1.egrid");
    test_assert_string_equal(f5.c_str(), "UPPER1.EGRID");
}

void test_file_list() {
    rd::util::TestArea ta("file_list");
    stringlist_type *s = stringlist_alloc_new();

    for (int i = 0; i < 10; i += 2) {
        std::string fname = rd::filename("case", FileType::RESTART, true, i);
        FILE *stream = util_fopen(fname.c_str(), "w");
        fclose(stream);
    }

    for (int i = 0; i < 10; i += 2) {
        char *fname = util_alloc_sprintf("Case.F%04d", i);
        FILE *stream = util_fopen(fname, "w");
        fclose(stream);
        free(fname);
    }

    for (int i = 0; i < 10; i += 2) {
        char *fname = util_alloc_sprintf("CaseMiXed.F%04d", i);
        FILE *stream = util_fopen(fname, "w");
        fclose(stream);
        free(fname);
    }

    rd_select_filelist(NULL, "case", FileType::RESTART, true, s);
    test_assert_int_equal(stringlist_get_size(s), 5);
    for (int i = 0; i < 5; i++) {
        std::string fname =
            rd::filename("case", FileType::RESTART, true, 2 * i);
        test_assert_string_equal(fname.c_str(), stringlist_iget(s, i));
    }

    rd_select_filelist(NULL, "CaseMiXed", FileType::RESTART, true, s);
    test_assert_int_equal(stringlist_get_size(s), 5);

    util_make_path("path");
    for (int i = 0; i < 10; i++) {
        std::string summary_file1 =
            rd::filename("path/CASE1", FileType::SUMMARY, false, i).string();
        std::string summary_file2 =
            rd::filename("path/CASE2", FileType::SUMMARY, true, i).string();
        std::string restart_file1 =
            rd::filename("path/CASE1", FileType::RESTART, false, i).string();

        FILE *f1 = fopen(summary_file1.c_str(), "w");
        fclose(f1);

        FILE *f2 = fopen(summary_file2.c_str(), "w");
        fclose(f2);

        FILE *f3 = fopen(restart_file1.c_str(), "w");
        fclose(f3);
    }
    printf("---------------------------------------------------------------\n");
    rd_select_filelist(NULL, "path/CASE1", FileType::SUMMARY, false, s);
    test_assert_int_equal(stringlist_get_size(s), 10);

    rd_select_filelist(NULL, "path/CASE1", FileType::SUMMARY, true, s);
    test_assert_int_equal(stringlist_get_size(s), 0);

    rd_select_filelist(NULL, "path/CASE2", FileType::SUMMARY, true, s);
    test_assert_int_equal(stringlist_get_size(s), 10);

    rd_select_filelist("path", "CASE1", FileType::SUMMARY, false, s);
    test_assert_int_equal(stringlist_get_size(s), 10);

    rd_select_filelist("path", "CASE1", FileType::SUMMARY, true, s);
    test_assert_int_equal(stringlist_get_size(s), 0);

    rd_select_filelist("path", "CASE2", FileType::SUMMARY, true, s);
    test_assert_int_equal(stringlist_get_size(s), 10);

    stringlist_free(s);
}

int main(int argc, char **argv) {
    test_filename_report_nr();
    test_filename_case();
    test_file_list();
}
