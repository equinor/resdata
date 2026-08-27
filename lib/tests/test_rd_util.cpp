#include <cstddef>
#include <cstdint>
#include <cstdlib>

#include <catch2/catch_message.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/generators/catch_generators_range.hpp>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <memory>
#include <new>
#include <string>
#include <utility>
#include <vector>

#include <resdata/rd_util.hpp>

#include "tmpdir.hpp"

TEST_CASE("Test file type format check", "[unittest]") {
    bool is_fmt = false;
    SECTION("A FUNRST file is a formatted file") {
        rd_get_file_type("CASE.FUNRST", &is_fmt, NULL);
        REQUIRE(is_fmt);
    }

    SECTION("A FMSPEC file is a formatted file") {
        rd_get_file_type("CASE.FMSPEC", &is_fmt, NULL);
        REQUIRE(is_fmt);
    }

    SECTION("A .F0001 file is a formatted file") {
        rd_get_file_type("CASE.F0001", &is_fmt, NULL);
        REQUIRE(is_fmt);
    }

    SECTION("A .X1234 file is an unformatted file") {
        rd_get_file_type("CASE.X1234", &is_fmt, NULL);
        REQUIRE(!is_fmt);
    }

    SECTION("A .UNSMRY file is an unformatted file") {
        rd_get_file_type("CASE.UNSMRY", &is_fmt, NULL);
        REQUIRE(!is_fmt);
    }
}

TEST_CASE("Test checked allocation size_t overflow guard", "[unittest]") {
    // A count that, when multiplied by sizeof(T), overflows size_t.
    const size_t overflowing = SIZE_MAX / sizeof(int) + 1;

    SECTION("checked_malloc throws bad_alloc on overflow") {
        REQUIRE_THROWS_AS(rd::checked_malloc<int>(overflowing), std::bad_alloc);
    }

    SECTION("checked_calloc throws bad_alloc on overflow") {
        REQUIRE_THROWS_AS(rd::checked_calloc<int>(overflowing), std::bad_alloc);
    }

    SECTION("checked_realloc throws bad_alloc on overflow") {
        auto ptr = rd::checked_malloc<int>(1);
        REQUIRE_THROWS_AS(rd::checked_realloc<int>(ptr, overflowing),
                          std::bad_alloc);
        // The original allocation must be preserved when realloc guard trips.
        REQUIRE(ptr != nullptr);
    }

    SECTION("Non-overflowing allocations succeed") {
        auto malloc_ptr = rd::checked_malloc<int>(4);
        REQUIRE(malloc_ptr != nullptr);

        auto calloc_ptr = rd::checked_calloc<int>(4);
        REQUIRE(calloc_ptr != nullptr);
        for (size_t i = 0; i < 4; ++i) {
            REQUIRE(calloc_ptr[i] == 0);
        }

        rd::checked_realloc<int>(malloc_ptr, 8);
        REQUIRE(malloc_ptr != nullptr);
    }
}

TEST_CASE("Test checked allocation of zero elements returns nullptr",
          "[unittest]") {
    SECTION("checked_malloc returns nullptr") {
        REQUIRE(rd::checked_malloc<int>(0) == nullptr);
    }

    SECTION("checked_calloc returns nullptr") {
        REQUIRE(rd::checked_calloc<int>(0) == nullptr);
    }

    SECTION("checked_realloc releases the existing allocation") {
        auto ptr = rd::checked_malloc<int>(4);
        REQUIRE(ptr != nullptr);
        rd::checked_realloc<int>(ptr, 0);
        REQUIRE(ptr == nullptr);
    }

    SECTION("checked_realloc of an empty pointer stays null") {
        std::unique_ptr<int[], void (*)(void *)> ptr{nullptr, &std::free};
        rd::checked_realloc<int>(ptr, 0);
        REQUIRE(ptr == nullptr);
    }
}

TEST_CASE("natural_compare orders embedded numbers by value", "[unittest]") {
    SECTION("shorter numbers sort before longer ones") {
        REQUIRE(rd::natural_compare("S9", "S10") < 0);
        REQUIRE(rd::natural_compare("S10", "S9") > 0);
    }

    SECTION("leading zeros do not change the numeric value") {
        REQUIRE(rd::natural_compare("S007", "S10") < 0);
        REQUIRE(rd::natural_compare("S0001", "S2") < 0);
    }

    SECTION("identical strings compare equal") {
        REQUIRE(rd::natural_compare("WWCT:OP-1", "WWCT:OP-1") == 0);
    }

    SECTION("strings differing only in leading zeros are ordered, not equal") {
        REQUIRE(rd::natural_compare("S07", "S7") != 0);
    }

    SECTION("a prefix sorts before the longer string") {
        REQUIRE(rd::natural_compare("FOP", "FOPT") < 0);
    }

    SECTION("sorting a summary key list is a natural order") {
        std::vector<std::string> keys{"S10", "S9", "S1", "S100", "S20"};
        std::sort(keys.begin(), keys.end(), rd::natural_less);
        REQUIRE(keys ==
                std::vector<std::string>{"S1", "S9", "S10", "S20", "S100"});
    }

    SECTION("the ordering is transitive") {
        REQUIRE(rd::natural_compare("9", "10") < 0);
        REQUIRE(rd::natural_compare("1x", "10") < 0);
        REQUIRE(rd::natural_compare("1x", "9") < 0);
    }
}

namespace {
/* Every string of length 0-4 over a deliberately adversarial alphabet: two
   distinct digits so that digit runs of differing value and differing length
   are generated, '0' so that leading zeros occur in every position, and one
   non-digit so that a digit run can be terminated by an ordinary character.*/
const std::vector<std::string> &adversarial_corpus() {
    static const std::vector<std::string> corpus = [] {
        const std::string alphabet = "01a";
        std::vector<std::string> all{""};
        std::vector<std::string> frontier{""};
        for (int length = 1; length <= 4; ++length) {
            std::vector<std::string> next;
            for (const auto &prefix : frontier)
                for (char c : alphabet)
                    next.push_back(prefix + c);
            all.insert(all.end(), next.begin(), next.end());
            frontier = std::move(next);
        }
        return all;
    }();
    return corpus;
}

std::string show(const std::string &s) { return "\"" + s + "\""; }
} // namespace

/* std::sort() requires its comparator to be a strict weak ordering; otherwise
   you get UB, with potential memory violations. The following
   test cases check the three defining properties exhaustively over the
   adversarial corpus.*/
TEST_CASE("natural_less is irreflexive", "[unittest]") {
    const auto &a = GENERATE(from_range(adversarial_corpus()));
    CAPTURE(a);
    REQUIRE_FALSE(rd::natural_less(a, a));
}

TEST_CASE("natural_less is asymmetric", "[unittest]") {
    const auto &a = GENERATE(from_range(adversarial_corpus()));

    std::string counterexample;
    for (const auto &b : adversarial_corpus()) {
        if (rd::natural_less(a, b) && rd::natural_less(b, a)) {
            counterexample = show(a) + " and " + show(b);
            break;
        }
    }

    INFO("both compare less than each other: " << counterexample);
    REQUIRE(counterexample.empty());
}

TEST_CASE("natural_less is transitive", "[unittest]") {
    const auto &a = GENERATE(from_range(adversarial_corpus()));
    const auto &corpus = adversarial_corpus();

    std::string counterexample;
    for (const auto &b : corpus) {
        if (!rd::natural_less(a, b))
            continue;
        for (const auto &c : corpus) {
            if (rd::natural_less(b, c) && !rd::natural_less(a, c)) {
                counterexample = show(a) + " < " + show(b) + " < " + show(c) +
                                 ", but not " + show(a) + " < " + show(c);
                break;
            }
        }
        if (!counterexample.empty())
            break;
    }

    INFO("transitivity violated: " << counterexample);
    REQUIRE(counterexample.empty());
}

TEST_CASE("natural_less has transitive equivalence", "[unittest]") {
    const auto &a = GENERATE(from_range(adversarial_corpus()));
    const auto &corpus = adversarial_corpus();

    auto equivalent = [](const std::string &x, const std::string &y) {
        return !rd::natural_less(x, y) && !rd::natural_less(y, x);
    };

    std::string counterexample;
    for (const auto &b : corpus) {
        if (!equivalent(a, b))
            continue;
        for (const auto &c : corpus) {
            if (equivalent(b, c) && !equivalent(a, c)) {
                counterexample = show(a) + " == " + show(b) + " == " + show(c) +
                                 ", but not " + show(a) + " == " + show(c);
                break;
            }
        }
        if (!counterexample.empty())
            break;
    }

    INFO("equivalence is not transitive: " << counterexample);
    REQUIRE(counterexample.empty());
}

TEST_CASE("natural_compare is antisymmetric in its sign", "[unittest]") {
    const auto &a = GENERATE(from_range(adversarial_corpus()));

    auto sign = [](int value) { return (value > 0) - (value < 0); };

    std::string counterexample;
    for (const auto &b : adversarial_corpus()) {
        if (sign(rd::natural_compare(a, b)) !=
            -sign(rd::natural_compare(b, a))) {
            counterexample = show(a) + " and " + show(b);
            break;
        }
    }

    INFO("comparing in the opposite order did not flip the sign: "
         << counterexample);
    REQUIRE(counterexample.empty());
}

TEST_CASE("natural_compare returns zero only for equal strings", "[unittest]") {
    const auto &a = GENERATE(from_range(adversarial_corpus()));

    std::string counterexample;
    for (const auto &b : adversarial_corpus()) {
        if ((rd::natural_compare(a, b) == 0) != (a == b)) {
            counterexample = show(a) + " and " + show(b);
            break;
        }
    }

    INFO("compared equal without being equal, or vice versa: "
         << counterexample);
    REQUIRE(counterexample.empty());
}

TEST_CASE("sorting the adversarial corpus yields a totally ordered sequence",
          "[unittest]") {
    auto corpus = adversarial_corpus();
    std::sort(corpus.begin(), corpus.end(), rd::natural_less);

    REQUIRE(std::is_sorted(corpus.begin(), corpus.end(), rd::natural_less));

    SECTION("every adjacent pair is strictly increasing") {
        /* The corpus holds no duplicates and natural_compare() is a total
           order, so no two neighbours may compare equivalent. */
        std::string counterexample;
        for (size_t i = 1; i < corpus.size(); ++i) {
            if (!rd::natural_less(corpus[i - 1], corpus[i])) {
                counterexample =
                    show(corpus[i - 1]) + " and " + show(corpus[i]);
                break;
            }
        }

        INFO(
            "adjacent entries are not strictly increasing: " << counterexample);
        REQUIRE(counterexample.empty());
    }
}

namespace {

void touch(const fs::path &file) {
    std::ofstream stream(file);
    if (!stream)
        throw std::runtime_error("Failed to create " + file.string());
}

/** The file names of @files, without the leading path, sorted so that the
   expectations do not depend on the order the directory is listed in. */
std::vector<std::string> filenames_of(const std::vector<std::string> &files) {
    std::vector<std::string> names;
    for (const auto &file : files)
        names.push_back(fs::path(file).filename().string());

    std::sort(names.begin(), names.end());
    return names;
}

} // namespace

TEST_CASE_METHOD(Tmpdir, "rd_select_filelist finds the extension of each type",
                 "[unittest]") {
    const auto [file_type, fmt_file, extension] =
        GENERATE(table<FileType, bool, std::string>({
            {FileType::UNIFIED_RESTART, true, "FUNRST"},
            {FileType::UNIFIED_RESTART, false, "UNRST"},
            {FileType::UNIFIED_SUMMARY, true, "FUNSMRY"},
            {FileType::UNIFIED_SUMMARY, false, "UNSMRY"},
            {FileType::GRID, true, "FGRID"},
            {FileType::GRID, false, "GRID"},
            {FileType::EGRID, true, "FEGRID"},
            {FileType::EGRID, false, "EGRID"},
            {FileType::INIT, true, "FINIT"},
            {FileType::INIT, false, "INIT"},
            {FileType::RFT, true, "FRFT"},
            {FileType::RFT, false, "RFT"},
            {FileType::DATA, true, "DATA"},
            {FileType::DATA, false, "DATA"},
        }));
    CAPTURE(static_cast<int>(file_type), fmt_file, extension);

    /* Every extension of every type, so that each selection has to pick its
       own out of all the others. */
    for (const auto *candidate :
         {"FUNRST", "UNRST", "FUNSMRY", "UNSMRY", "FGRID", "GRID", "FEGRID",
          "EGRID", "FINIT", "INIT", "FRFT", "RFT", "DATA"})
        touch(dirname / ("CASE." + std::string(candidate)));

    auto files = rd_select_filelist(dirname.string().c_str(), "CASE", file_type,
                                    fmt_file);

    REQUIRE(filenames_of(files) ==
            std::vector<std::string>{"CASE." + extension});
}

TEST_CASE_METHOD(Tmpdir, "rd_select_filelist selects every file for OTHER",
                 "[unittest]") {
    touch(dirname / "CASE.EGRID");
    touch(dirname / "CASE.UNSMRY");
    touch(dirname / "CASE.txt");
    touch(dirname / "OTHER.EGRID");

    const bool fmt_file = GENERATE(false, true);
    CAPTURE(fmt_file);

    auto files = rd_select_filelist(dirname.string().c_str(), "CASE",
                                    FileType::OTHER, fmt_file);

    REQUIRE(filenames_of(files) ==
            std::vector<std::string>{"CASE.EGRID", "CASE.UNSMRY", "CASE.txt"});
}

TEST_CASE_METHOD(Tmpdir, "rd_select_filelist follows the case of the base",
                 "[unittest]") {
    if (is_case_insensitive(dirname))
        SKIP("On a case-insensitive filesystem");

    const auto [base, expected] = GENERATE(table<std::string, std::string>({
        {"CASE", "CASE.EGRID"},
        {"case", "case.egrid"},
        {"CaseMiXed", "CaseMiXed.EGRID"},
        {"", ".EGRID"},
        {"123", "123.EGRID"},
    }));
    CAPTURE(base, expected);

    for (const auto &name : {"CASE", "case", "CaseMiXed", "", "123"}) {
        touch(dirname / (std::string(name) + ".EGRID"));
        touch(dirname / (std::string(name) + ".egrid"));
    }

    auto files = rd_select_filelist(dirname.string().c_str(), base,
                                    FileType::EGRID, false);

    REQUIRE(filenames_of(files) == std::vector<std::string>{expected});
}

TEST_CASE_METHOD(Tmpdir, "rd_select_filelist treats the base as a pattern",
                 "[unittest]") {
    touch(dirname / "CASE.EGRID");
    touch(dirname / "CASE1.EGRID");
    touch(dirname / "OTHER.EGRID");
    touch(dirname / ".EGRID");

    const std::string path = dirname.string();

    SECTION("a base is matched in full") {
        auto files =
            rd_select_filelist(path.c_str(), "CASE", FileType::EGRID, false);

        REQUIRE(filenames_of(files) == std::vector<std::string>{"CASE.EGRID"});
    }

    SECTION("an empty base selects the extension alone, not every case") {
        auto files =
            rd_select_filelist(path.c_str(), "", FileType::EGRID, false);

        REQUIRE(filenames_of(files) == std::vector<std::string>{".EGRID"});
    }

    SECTION("a wildcard base selects every case") {
        auto files =
            rd_select_filelist(path.c_str(), "*", FileType::EGRID, false);

        REQUIRE(filenames_of(files) == std::vector<std::string>{"CASE.EGRID",
                                                                "CASE1.EGRID",
                                                                "OTHER.EGRID"});
    }

    SECTION("a wildcard matches part of the base") {
        auto files =
            rd_select_filelist(path.c_str(), "CASE*", FileType::EGRID, false);

        REQUIRE(filenames_of(files) ==
                std::vector<std::string>{"CASE.EGRID", "CASE1.EGRID"});
    }
}

TEST_CASE_METHOD(Tmpdir, "rd_select_filelist selects numbered report files",
                 "[unittest]") {
    const auto [file_type, fmt_file, leading_char] =
        GENERATE(table<FileType, bool, char>({
            {FileType::SUMMARY, true, 'A'},
            {FileType::SUMMARY, false, 'S'},
            {FileType::RESTART, true, 'F'},
            {FileType::RESTART, false, 'X'},
        }));
    CAPTURE(static_cast<int>(file_type), fmt_file, leading_char);

    for (const char candidate : {'A', 'S', 'F', 'X'})
        for (int report = 0; report < 3; report++)
            touch(dirname / ("CASE." + std::string(1, candidate) + "000" +
                             std::to_string(report)));

    auto files = rd_select_filelist(dirname.string().c_str(), "CASE", file_type,
                                    fmt_file);

    const std::string ext = "CASE." + std::string(1, leading_char) + "000";
    REQUIRE(filenames_of(files) ==
            std::vector<std::string>{ext + "0", ext + "1", ext + "2"});
}

TEST_CASE_METHOD(Tmpdir, "rd_select_filelist matches the report base in full",
                 "[unittest]") {
    for (int report = 0; report < 3; report++) {
        const std::string ext = ".S000" + std::to_string(report);
        touch(dirname / ("CASE" + ext));
        touch(dirname / ("CASE10" + ext));
    }
    touch(dirname / ".S0000");

    const std::string path = dirname.string();

    SECTION("a base is not treated as a prefix of a longer case name") {
        auto files =
            rd_select_filelist(path.c_str(), "CASE", FileType::SUMMARY, false);

        REQUIRE(filenames_of(files) == std::vector<std::string>{"CASE.S0000",
                                                                "CASE.S0001",
                                                                "CASE.S0002"});
    }

    SECTION("an empty base selects the extension alone, not every case") {
        auto files =
            rd_select_filelist(path.c_str(), "", FileType::SUMMARY, false);

        REQUIRE(filenames_of(files) == std::vector<std::string>{".S0000"});
    }

    SECTION("a wildcard is not expanded for numbered report files") {
        auto files =
            rd_select_filelist(path.c_str(), "*", FileType::SUMMARY, false);

        REQUIRE(files.empty());
    }
}

TEST_CASE_METHOD(Tmpdir, "rd_select_filelist rejects malformed report numbers",
                 "[unittest]") {
    touch(dirname / "CASE.S0000");
    touch(dirname / "CASE.S000");     /* Too short */
    touch(dirname / "CASE.S00000");   /* Too long */
    touch(dirname / "CASE.S000x");    /* Not a number */
    touch(dirname / "CASE.s0000");    /* Wrong case */
    touch(dirname / "CASE.X0000");    /* Another file type */
    touch(dirname / "CASE.0000");     /* No leading character */
    touch(dirname / "CASEX.S0000");   /* Another case */
    touch(dirname / "CASE.S0000.gz"); /* Not the last extension */

    auto files = rd_select_filelist(dirname.string().c_str(), "CASE",
                                    FileType::SUMMARY, false);

    REQUIRE(filenames_of(files) == std::vector<std::string>{"CASE.S0000"});
}

TEST_CASE_METHOD(Tmpdir, "rd_select_filelist sorts report files by number",
                 "[unittest]") {
    /* Created out of order, and spanning a digit count so that a bytewise sort
       would put S9 after S10. */
    for (const int report : {10, 2, 9, 0, 100})
        touch(dirname /
              ("CASE.S" + std::string(4 - std::to_string(report).size(), '0') +
               std::to_string(report)));

    auto files = rd_select_filelist(dirname.string().c_str(), "CASE",
                                    FileType::SUMMARY, false);

    std::vector<std::string> names;
    for (const auto &file : files)
        names.push_back(fs::path(file).filename().string());

    REQUIRE(names == std::vector<std::string>{"CASE.S0000", "CASE.S0002",
                                              "CASE.S0009", "CASE.S0010",
                                              "CASE.S0100"});
}

TEST_CASE_METHOD(Tmpdir, "rd_select_filelist prefixes the results with a path",
                 "[unittest]") {
    touch(dirname / "CASE.EGRID");
    touch(dirname / "CASE.S0000");

    SECTION("the path is prepended when it is passed separately") {
        auto files = rd_select_filelist(dirname.string().c_str(), "CASE",
                                        FileType::EGRID, false);

        REQUIRE(files == std::vector<std::string>{(dirname / "CASE.EGRID")});
    }

    SECTION("the path may be carried by the base instead") {
        const std::string base = (dirname / "CASE").string();

        auto files = rd_select_filelist(nullptr, base, FileType::EGRID, false);

        REQUIRE(files == std::vector<std::string>{(dirname / "CASE.EGRID")});
    }

    SECTION("a base carrying the path also works for report files") {
        const std::string base = (dirname / "CASE").string();

        auto files =
            rd_select_filelist(nullptr, base, FileType::SUMMARY, false);

        REQUIRE(files == std::vector<std::string>{(dirname / "CASE.S0000")});
    }
}

TEST_CASE_METHOD(Tmpdir, "rd_select_filelist selects nothing", "[unittest]") {
    const std::string missing = (dirname / "no_such_directory").string();

    SECTION("a missing directory yields no files") {
        REQUIRE(
            rd_select_filelist(missing.c_str(), "CASE", FileType::EGRID, false)
                .empty());
        REQUIRE(rd_select_filelist(missing.c_str(), "CASE", FileType::SUMMARY,
                                   false)
                    .empty());
    }

    SECTION("an empty directory yields no files") {
        REQUIRE(rd_select_filelist(dirname.string().c_str(), "CASE",
                                   FileType::EGRID, false)
                    .empty());
        REQUIRE(rd_select_filelist(dirname.string().c_str(), "CASE",
                                   FileType::SUMMARY, false)
                    .empty());
    }
}

TEST_CASE_METHOD(Tmpdir, "select_matching_files expands a glob pattern",
                 "[unittest]") {
    touch(dirname / "CASE.EGRID");
    touch(dirname / "CASE.UNSMRY");
    touch(dirname / "OTHER.EGRID");
    fs::create_directory(dirname / "subdir");

    const std::string path = dirname.string();

    SECTION("a pattern without wildcards selects a single file") {
        REQUIRE(filenames_of(select_matching_files(path, "CASE.EGRID")) ==
                std::vector<std::string>{"CASE.EGRID"});
    }

    SECTION("a wildcard selects every match") {
        REQUIRE(filenames_of(select_matching_files(path, "*.EGRID")) ==
                std::vector<std::string>{"CASE.EGRID", "OTHER.EGRID"});
    }

    SECTION("a single character wildcard matches one character") {
        REQUIRE(filenames_of(select_matching_files(path, "CASE.?GRID")) ==
                std::vector<std::string>{"CASE.EGRID"});
    }

    SECTION("a pattern without matches yields no files") {
        REQUIRE(select_matching_files(path, "*.INIT").empty());
    }

    SECTION("a missing directory yields no files") {
        REQUIRE(
            select_matching_files((dirname / "missing").string(), "*").empty());
    }

    SECTION("the results are prefixed with the path") {
        REQUIRE(select_matching_files(path, "CASE.EGRID") ==
                std::vector<std::string>{(dirname / "CASE.EGRID")});
    }
}
