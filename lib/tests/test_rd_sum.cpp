#include <catch2/catch_all.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

#include <ert/util/stringlist.hpp>
#include <ert/util/time_t_vector.hpp>
#include <ert/util/util.hpp>

#include <resdata/rd_file.hpp>
#include <resdata/rd_kw.hpp>
#include <resdata/rd_kw_magic.hpp>
#include <resdata/rd_smspec.hpp>
#include <resdata/rd_sum.hpp>
#include <resdata/rd_sum_tstep.hpp>

#include "tmpdir.hpp"

using namespace Catch;
using namespace Matchers;

namespace {

struct WriteSpec {
    time_t start_time = util_make_date_utc(1, 1, 2010);
    int nx = 10;
    int ny = 11;
    int nz = 12;
    int num_report_steps = 5;
    int num_ministep = 10;
    double start_seconds = 0.0;
    double ministep_length = 86400.0; // one day in seconds
};

/**
 * Write a summary case with three variables (FOPT, BPR:567, WWCT:OP-1) to the
 * given case path. Returns the simulated end time as time_t.
 */
time_t write_test_summary(const std::string &case_path, const WriteSpec &spec,
                          bool fmt_output, bool unified) {
    auto rd_sum = rd_sum_ptr(
        rd_sum_alloc_writer(case_path.c_str(), fmt_output, unified, ":",
                            spec.start_time, true, spec.nx, spec.ny, spec.nz),
        &rd_sum_free);

    rd_smspec_type *smspec = rd_sum_get_smspec(rd_sum.get());
    const rd::smspec_node *fopt =
        rd_smspec_add_node(smspec, "FOPT", "SM3", 99.0f);
    const rd::smspec_node *bpr =
        rd_smspec_add_node(smspec, "BPR", 567, "BARS", 0.0f);
    const rd::smspec_node *wwct =
        rd_smspec_add_node(smspec, "WWCT", "OP-1", "(1)", 0.0f);

    double sim_seconds = spec.start_seconds;
    for (int report_step = 0; report_step < spec.num_report_steps;
         ++report_step) {
        for (int step = 0; step < spec.num_ministep; ++step) {
            rd_sum_tstep_type *tstep =
                rd_sum_add_tstep(rd_sum.get(), report_step + 1, sim_seconds);
            rd_sum_tstep_set_from_node(tstep, *fopt, sim_seconds);
            rd_sum_tstep_set_from_node(tstep, *bpr, 10.0 * sim_seconds);
            rd_sum_tstep_set_from_node(tstep, *wwct, 100.0 * sim_seconds);
            sim_seconds += spec.ministep_length;
        }
    }
    rd_sum_fwrite(rd_sum.get());

    time_t end_time = spec.start_seconds + spec.start_time;
    util_inplace_forward_seconds_utc(
        &end_time,
        (spec.num_report_steps * spec.num_ministep - 1) * spec.ministep_length);
    return end_time;
}

} // namespace

TEST_CASE_METHOD(Tmpdir, "Read summary written by writer") {
    GIVEN("A summary case") {
        const bool fmt_output = GENERATE(true, false);
        const bool unified = GENERATE(true, false);
        WriteSpec spec;
        auto case_path = (dirname / "CASE").string();
        const time_t end_time =
            write_test_summary(case_path, spec, fmt_output, unified);

        auto rd_sum = read_summary(case_path, ":", !fmt_output);

        SECTION("time range") {
            REQUIRE(rd_sum_get_start_time(rd_sum.get()) == spec.start_time);
            REQUIRE(rd_sum_get_data_start(rd_sum.get()) == spec.start_time);
            REQUIRE(rd_sum_get_end_time(rd_sum.get()) == end_time);
        }

        SECTION("report steps") {
            REQUIRE(rd_sum_get_first_report_step(rd_sum.get()) == 1);
            REQUIRE(rd_sum_get_last_report_step(rd_sum.get()) ==
                    spec.num_report_steps);
        }

        THEN("data length matches the number of written ministeps") {
            REQUIRE(rd_sum_get_data_length(rd_sum.get()) ==
                    spec.num_report_steps * spec.num_ministep);
        }

        THEN("keys are present") {
            REQUIRE(rd_sum_has_key(rd_sum.get(), "FOPT"));
            REQUIRE(rd_sum_has_key(rd_sum.get(), "BPR:567"));
            REQUIRE(rd_sum_has_key(rd_sum.get(), "WWCT:OP-1"));
            REQUIRE(rd_sum_has_general_var(rd_sum.get(), "FOPT"));
            REQUIRE_FALSE(rd_sum_has_key(rd_sum.get(), "NO_SUCH_KEY"));
        }

        THEN("units are stored") {
            REQUIRE(std::string(rd_sum_get_unit(rd_sum.get(), "FOPT")) ==
                    "SM3");
            REQUIRE(std::string(rd_sum_get_unit(rd_sum.get(), "BPR:567")) ==
                    "BARS");
            REQUIRE(std::string(rd_sum_get_unit(rd_sum.get(), "WWCT:OP-1")) ==
                    "(1)");
        }

        THEN("first and last values agree with the linear model") {
            const double last_sim_seconds =
                (spec.num_report_steps * spec.num_ministep - 1) *
                spec.ministep_length;

            REQUIRE_THAT(rd_sum_get_first_value_gen_key(rd_sum.get(), "FOPT"),
                         WithinAbs(0.0, 1e-6));
            REQUIRE_THAT(rd_sum_get_last_value_gen_key(rd_sum.get(), "FOPT"),
                         WithinAbs(last_sim_seconds, 1e-3));
            REQUIRE_THAT(rd_sum_get_last_value_gen_key(rd_sum.get(), "BPR:567"),
                         WithinAbs(10.0 * last_sim_seconds, 1e-3));
            REQUIRE_THAT(
                rd_sum_get_last_value_gen_key(rd_sum.get(), "WWCT:OP-1"),
                WithinAbs(100.0 * last_sim_seconds, 1e-3));
        }

        THEN("iget returns the expected time at every step") {
            const int n = rd_sum_get_data_length(rd_sum.get());
            for (int i = 0; i < n; ++i) {
                const double expected = i * spec.ministep_length;
                REQUIRE_THAT(rd_sum_get_general_var(rd_sum.get(), i, "FOPT"),
                             WithinAbs(expected, 1e-3));
                REQUIRE_THAT(rd_sum_iget_sim_days(rd_sum.get(), i),
                             WithinAbs(expected / 86400.0, 1e-6));

                time_t expected_time = spec.start_time;
                util_inplace_forward_seconds_utc(&expected_time, expected);
                REQUIRE(rd_sum_iget_sim_time(rd_sum.get(), i) == expected_time);
            }
        }

        THEN("missing keys throw out_of_range") {
            REQUIRE_THROWS_AS(
                rd_sum_get_general_var(rd_sum.get(), 0, "NO_SUCH_KEY"),
                std::out_of_range);
        }

        THEN("matching general var list collects keys") {
            auto list =
                std::unique_ptr<stringlist_type, void (*)(stringlist_type *)>(
                    rd_sum_alloc_matching_general_var_list(rd_sum.get(), "F*"),
                    &stringlist_free);
            REQUIRE(stringlist_get_size(list.get()) == 1);
            REQUIRE(std::string(stringlist_iget(list.get(), 0)) == "FOPT");
        }

        THEN("well list returns the producer") {
            auto wells =
                std::unique_ptr<stringlist_type, void (*)(stringlist_type *)>(
                    rd_sum_alloc_well_list(rd_sum.get(), nullptr),
                    &stringlist_free);
            REQUIRE(stringlist_get_size(wells.get()) == 1);
            REQUIRE(std::string(stringlist_iget(wells.get(), 0)) == "OP-1");
        }

        THEN("sim_time at the end of the first report step maps to step 1") {
            time_t mid = spec.start_time;
            util_inplace_forward_seconds_utc(&mid, (spec.num_ministep - 1) *
                                                       spec.ministep_length);
            REQUIRE(rd_sum_check_sim_time(rd_sum.get(), mid));
            REQUIRE(rd_sum_get_report_step_from_time(rd_sum.get(), mid) == 1);
        }

        THEN("sim_time before start is rejected") {
            time_t before = spec.start_time;
            util_inplace_forward_seconds_utc(&before, -spec.ministep_length);
            REQUIRE_FALSE(rd_sum_check_sim_time(rd_sum.get(), before));
        }

        THEN("sim_days within range is accepted") {
            REQUIRE(rd_sum_check_sim_days(rd_sum.get(), 0.0));
            REQUIRE_FALSE(rd_sum_check_sim_days(rd_sum.get(), -1.0));
            REQUIRE_FALSE(rd_sum_check_sim_days(rd_sum.get(),
                                                spec.num_report_steps *
                                                    spec.num_ministep * 10.0));
        }
        THEN("Path accessors point at the written case") {
            REQUIRE(std::string(rd_sum_get_base(rd_sum.get())) == "CASE");
            REQUIRE(rd_sum_get_restart_case(rd_sum.get()) == nullptr);
        }

        SECTION("Allocated time and data vectors") {
            auto times = std::unique_ptr<time_t_vector_type,
                                         void (*)(time_t_vector_type *)>(
                rd_sum_alloc_time_vector(rd_sum.get(), false),
                &time_t_vector_free);
            REQUIRE(time_t_vector_size(times.get()) ==
                    rd_sum_get_data_length(rd_sum.get()));
            REQUIRE(time_t_vector_iget(times.get(), 0) == spec.start_time);

            const int param_index =
                rd_sum_get_general_var_params_index(rd_sum.get(), "FOPT");
            auto data = std::unique_ptr<double_vector_type,
                                        void (*)(double_vector_type *)>(
                rd_sum_alloc_data_vector(rd_sum.get(), param_index, false),
                &double_vector_free);
            REQUIRE(double_vector_size(data.get()) ==
                    rd_sum_get_data_length(rd_sum.get()));
            REQUIRE_THAT(double_vector_iget(data.get(), 0),
                         WithinAbs(0.0, 1e-6));
        }
    }
}

TEST_CASE_METHOD(Tmpdir, "Loading a case with data before its parent in time") {
    WriteSpec spec;
    spec.start_seconds = 1.0;
    auto base_path = (dirname / "CASE1").string();
    write_test_summary(base_path, spec, /*fmt_output=*/false,
                       /*unified=*/true);
    auto restart_path = (dirname / "CASE2").string();
    constexpr double child_fopt_value = 1.0e9;
    {
        auto restart_sum = rd_sum_ptr(
            rd_sum_alloc_restart_writer(restart_path.c_str(), base_path.c_str(),
                                        /*fmt_output=*/false,
                                        /*unified=*/true, ":", spec.start_time,
                                        true, spec.nx, spec.ny, spec.nz),
            &rd_sum_free);
        rd_smspec_type *smspec = rd_sum_get_smspec(restart_sum.get());
        const rd::smspec_node *fopt =
            rd_smspec_add_node(smspec, "FOPT", "SM3", 99.0f);
        // The child case has a single ministep at sim_seconds = 0, i.e.
        // before the data_start in the parent.
        rd_sum_tstep_type *tstep = rd_sum_add_tstep(restart_sum.get(), 1, 0.0);
        rd_sum_tstep_set_from_node(tstep, *fopt, child_fopt_value);
        rd_sum_fwrite(restart_sum.get());
    }

    auto rd_sum = read_summary(restart_path);
    REQUIRE(rd_sum != nullptr);
    REQUIRE(rd_sum_has_key(rd_sum.get(), "FOPT"));

    const int length = rd_sum_get_data_length(rd_sum.get());
    REQUIRE(length > 0);

    REQUIRE_THAT(rd_sum_get_general_var(rd_sum.get(), 0, "FOPT"),
                 WithinAbs(child_fopt_value, 1e-3));
    for (int i = 1; i < length; ++i) {
        REQUIRE_THAT(
            rd_sum_get_general_var(rd_sum.get(), i, "FOPT"),
            WithinAbs(spec.start_seconds + (i - 1) * spec.ministep_length,
                      1e-3));
    }
}

TEST_CASE_METHOD(Tmpdir, "Restart writer writes has restart kw") {
    const bool fmt_output = GENERATE(true, false);
    const bool unified = GENERATE(true, false);
    WriteSpec spec;
    auto base_path = (dirname / "CASE1").string();
    write_test_summary(base_path, spec, fmt_output, unified);

    auto restart_path = (dirname / "CASE2").string();
    {
        auto restart_sum = rd_sum_ptr(
            rd_sum_alloc_restart_writer(
                restart_path.c_str(), base_path.c_str(), fmt_output, unified,
                ":", spec.start_time, true, spec.nx, spec.ny, spec.nz),
            &rd_sum_free);
        rd_smspec_type *smspec = rd_sum_get_smspec(restart_sum.get());
        const rd::smspec_node *fopt =
            rd_smspec_add_node(smspec, "FOPT", "SM3", 99.0f);
        rd_sum_tstep_type *tstep = rd_sum_add_tstep(restart_sum.get(), 1, 0.0);
        rd_sum_tstep_set_from_node(tstep, *fopt, 0.0);
        rd_sum_fwrite(restart_sum.get());
    }

    const std::string smspec_ext = fmt_output ? ".FSMSPEC" : ".SMSPEC";
    auto restart_file = rd_file_ptr(
        rd_file_open((restart_path + smspec_ext).c_str(), 0), &rd_file_close);
    REQUIRE(restart_file != nullptr);
    rd_file_view_type *view = rd_file_get_global_view(restart_file.get());
    REQUIRE(rd_file_view_has_kw(view, RESTART_KW));
}

namespace {

double expected_last_fopt(const WriteSpec &spec) {
    return (spec.num_report_steps * spec.num_ministep - 1) *
           spec.ministep_length;
}

// Make `path` look older than `reference`
void make_older(const fs::path &path, const fs::path &reference) {
    auto ref_time = fs::last_write_time(reference);
    fs::last_write_time(path, ref_time - std::chrono::seconds(10));
}

} // namespace

TEST_CASE_METHOD(Tmpdir, "Explicit extension picks specific summary header") {
    WriteSpec binary_spec;
    binary_spec.num_report_steps = 5;
    WriteSpec fmt_spec;
    fmt_spec.num_report_steps = 3;

    auto case_path = (dirname / "CASE").string();
    write_test_summary(case_path, binary_spec, false, true);
    write_test_summary(case_path, fmt_spec, true, true);

    SECTION("'.SMSPEC' selects the binary case") {
        auto rd_sum = read_summary(case_path + ".SMSPEC");
        REQUIRE(rd_sum != nullptr);
        REQUIRE_THAT(rd_sum_get_last_value_gen_key(rd_sum.get(), "FOPT"),
                     WithinAbs(expected_last_fopt(binary_spec), 1e-3));
    }

    SECTION("'.FSMSPEC' selects the formatted case") {
        auto rd_sum =
            read_summary(case_path + ".FSMSPEC", ":", /*lazy_load=*/false);
        REQUIRE(rd_sum != nullptr);
        REQUIRE_THAT(rd_sum_get_last_value_gen_key(rd_sum.get(), "FOPT"),
                     WithinAbs(expected_last_fopt(fmt_spec), 1e-3));
    }

    SECTION("'.UNSMRY' selects the binary case via the data file") {
        auto rd_sum = read_summary(case_path + ".UNSMRY");
        REQUIRE(rd_sum != nullptr);
        REQUIRE_THAT(rd_sum_get_last_value_gen_key(rd_sum.get(), "FOPT"),
                     WithinAbs(expected_last_fopt(binary_spec), 1e-3));
    }
}

TEST_CASE_METHOD(Tmpdir,
                 "Without an extension and ambiguous summary headers the "
                 "older header is chosen") {
    // When both .SMSPEC and .FSMSPEC exist the current implementation picks
    // the OLDER of the two even though it claims it picks the newer.
    WriteSpec binary_spec;
    binary_spec.num_report_steps = 5;
    WriteSpec fmt_spec;
    fmt_spec.num_report_steps = 3;

    auto case_path = (dirname / "CASE").string();
    write_test_summary(case_path, binary_spec, false, true);
    write_test_summary(case_path, fmt_spec, true, true);

    SECTION("binary newer than formatted -> formatted is loaded") {
        make_older(dirname / "CASE.FSMSPEC", dirname / "CASE.SMSPEC");
        auto rd_sum = read_summary(case_path, ":", /*lazy_load=*/false);
        REQUIRE(rd_sum != nullptr);
        REQUIRE_THAT(rd_sum_get_last_value_gen_key(rd_sum.get(), "FOPT"),
                     WithinAbs(expected_last_fopt(fmt_spec), 1e-3));
    }

    SECTION("formatted newer than binary -> binary is loaded") {
        make_older(dirname / "CASE.SMSPEC", dirname / "CASE.FSMSPEC");
        auto rd_sum = read_summary(case_path);
        REQUIRE(rd_sum != nullptr);
        REQUIRE_THAT(rd_sum_get_last_value_gen_key(rd_sum.get(), "FOPT"),
                     WithinAbs(expected_last_fopt(binary_spec), 1e-3));
    }
}

TEST_CASE_METHOD(Tmpdir, "Without extension the newest data files are chosen") {
    WriteSpec unified_spec;
    unified_spec.num_report_steps = 5;
    WriteSpec split_spec;
    split_spec.num_report_steps = 3;

    auto case_path = (dirname / "CASE").string();
    write_test_summary(case_path, unified_spec, false, true);

    fs::copy_file(dirname / "CASE.SMSPEC", dirname / "CASE.SMSPEC.bak");
    write_test_summary(case_path, split_spec, false, false);
    fs::remove(dirname / "CASE.SMSPEC");
    fs::rename(dirname / "CASE.SMSPEC.bak", dirname / "CASE.SMSPEC");

    SECTION("unified newer than split picks unified") {
        for (int n = 1; n <= split_spec.num_report_steps; ++n) {
            char ext[8];
            sprintf(ext, ".S%04d", n);
            make_older(dirname / ("CASE" + std::string(ext)),
                       dirname / "CASE.UNSMRY");
        }
        auto rd_sum = read_summary(case_path);
        REQUIRE(rd_sum != nullptr);
        REQUIRE(rd_sum_get_data_length(rd_sum.get()) ==
                unified_spec.num_report_steps * unified_spec.num_ministep);
    }

    SECTION("split newer than unified picks split") {
        make_older(dirname / "CASE.UNSMRY",
                   dirname / ("CASE.S000" +
                              std::to_string(split_spec.num_report_steps)));
        auto rd_sum = read_summary(case_path);
        REQUIRE(rd_sum != nullptr);
        REQUIRE(rd_sum_get_data_length(rd_sum.get()) ==
                split_spec.num_report_steps * split_spec.num_ministep);
    }
}

TEST_CASE_METHOD(Tmpdir, "Filenames with two extensions", "[unittest]") {
    WriteSpec spec;
    auto plain_path = (dirname / "BASE").string();
    write_test_summary(plain_path, spec, false, true);

    fs::rename(dirname / "BASE.SMSPEC", dirname / "BASE.compressed.SMSPEC");
    fs::rename(dirname / "BASE.UNSMRY", dirname / "BASE.compressed.UNSMRY");
    REQUIRE(fs::exists(dirname / "BASE.compressed.SMSPEC"));
    REQUIRE(fs::exists(dirname / "BASE.compressed.UNSMRY"));

    auto two_ext_path = (dirname / "BASE.compressed").string();

    SECTION("basename is BASE.compressed") {
        auto extension = GENERATE(".SMSPEC", ".UNSMRY");
        auto rd_sum = read_summary(two_ext_path + extension);
        REQUIRE(rd_sum != nullptr);
        REQUIRE(std::string(rd_sum_get_base(rd_sum.get())) ==
                "BASE.compressed");
    }

    SECTION("dropping the recognized extension fails because '.compressed' is "
            "parsed as the extension") {
        auto rd_sum = read_summary(two_ext_path);
        REQUIRE(rd_sum == nullptr);
    }
}

TEST_CASE_METHOD(Tmpdir, "Summary case inside a directory whose name has dot") {
    auto subdir = dirname / "results.v1.0";
    fs::create_directories(subdir);
    auto case_path = (subdir / "CASE").string();

    WriteSpec spec;
    write_test_summary(case_path, spec, false, true);
    REQUIRE(fs::exists(subdir / "CASE.SMSPEC"));

    SECTION("read without extension") {
        auto rd_sum = read_summary(case_path);
        REQUIRE(rd_sum != nullptr);
        REQUIRE(std::string(rd_sum_get_base(rd_sum.get())) == "CASE");
        REQUIRE_THAT(rd_sum_get_last_value_gen_key(rd_sum.get(), "FOPT"),
                     WithinAbs(expected_last_fopt(spec), 1e-3));
    }

    SECTION("read with explicit .SMSPEC") {
        auto rd_sum = read_summary(case_path + ".SMSPEC");
        REQUIRE(rd_sum != nullptr);
        REQUIRE(std::string(rd_sum_get_base(rd_sum.get())) == "CASE");
    }
}

TEST_CASE_METHOD(Tmpdir, "rd_sum getters preserve path/base/case relations") {
    auto subdir = dirname / "casedir";
    fs::create_directories(subdir);
    WriteSpec spec;
    write_test_summary((subdir / "CASE").string(), spec, /*fmt=*/false,
                       /*unified=*/true);

    SECTION("input without a directory: path is null and abs_path is cwd") {
        auto previous_cwd = fs::current_path();
        fs::current_path(subdir);
        auto rd_sum = read_summary("CASE");
        fs::current_path(previous_cwd);

        REQUIRE(rd_sum != nullptr);
        REQUIRE(rd_sum_get_path(rd_sum.get()) == nullptr);
        REQUIRE(std::string(rd_sum_get_base(rd_sum.get())) == "CASE");
        REQUIRE(std::string(rd_sum_get_case(rd_sum.get())) == "CASE");
        const char *abs_path = rd_sum_get_abs_path(rd_sum.get());
        REQUIRE(abs_path != nullptr);
        REQUIRE(fs::path(abs_path).is_absolute());
        REQUIRE(fs::equivalent(abs_path, subdir));
    }

    SECTION("input with a directory: path/base/case/abs_path are populated") {
        auto rd_sum = read_summary((subdir / "CASE").string());
        REQUIRE(rd_sum != nullptr);
        REQUIRE(rd_sum_get_path(rd_sum.get()) != nullptr);
        REQUIRE(fs::equivalent(rd_sum_get_path(rd_sum.get()), subdir));
        REQUIRE(std::string(rd_sum_get_base(rd_sum.get())) == "CASE");
        REQUIRE(std::string(rd_sum_get_case(rd_sum.get())) ==
                (subdir / "CASE").string());
        REQUIRE(fs::path(rd_sum_get_abs_path(rd_sum.get())).is_absolute());
    }
}

TEST_CASE_METHOD(Tmpdir, "get_case is the same regardless of input extension") {
    WriteSpec spec;
    auto case_path = (dirname / "CASE").string();
    write_test_summary(case_path, spec, /*fmt=*/false, /*unified=*/true);
    {
        std::ofstream ofs(dirname / "CASE.DATA");
        ofs << "RUNSPEC\n";
    }

    const std::vector<std::string> inputs = {
        case_path,
        case_path + ".SMSPEC",
        case_path + ".UNSMRY",
        case_path + ".DATA",
    };
    for (const auto &input : inputs) {
        INFO("input=" << input);
        auto rd_sum = read_summary(input);
        REQUIRE(rd_sum != nullptr);
        REQUIRE(std::string(rd_sum_get_case(rd_sum.get())) == case_path);
        REQUIRE(std::string(rd_sum_get_base(rd_sum.get())) == "CASE");
    }
}

TEST_CASE_METHOD(Tmpdir,
                 "Unknown extensions are ignored when locating SMSPEC") {
    WriteSpec spec;
    auto case_path = (dirname / "CASE").string();
    write_test_summary(case_path, spec, /*fmt=*/false, /*unified=*/true);

    auto rd_sum = read_summary(case_path + ".LOG");
    REQUIRE(rd_sum != nullptr);
    REQUIRE(std::string(rd_sum_get_base(rd_sum.get())) == "CASE");
    REQUIRE_THAT(rd_sum_get_last_value_gen_key(rd_sum.get(), "FOPT"),
                 WithinAbs(expected_last_fopt(spec), 1e-3));
}

TEST_CASE_METHOD(Tmpdir,
                 "Explicit .FSMSPEC with only .SMSPEC on disk returns null") {
    WriteSpec spec;
    auto case_path = (dirname / "CASE").string();
    write_test_summary(case_path, spec, /*fmt=*/false, /*unified=*/true);
    REQUIRE_FALSE(fs::exists(dirname / "CASE.FSMSPEC"));

    auto rd_sum = read_summary(case_path + ".FSMSPEC");
    REQUIRE(rd_sum == nullptr);
}

TEST_CASE_METHOD(Tmpdir, "Missing case returns null") {
    auto rd_sum = read_summary((dirname / "DOES_NOT_EXIST").string());
    REQUIRE(rd_sum == nullptr);
}

TEST_CASE_METHOD(Tmpdir, "rd_sum_fwrite writes SMSPEC at rd_case") {
    const bool fmt_output = GENERATE(true, false);
    const bool unified = GENERATE(true, false);

    auto case_path = (dirname / "CASE").string();
    WriteSpec spec;
    write_test_summary(case_path, spec, fmt_output, unified);

    auto rd_sum = read_summary(case_path);
    REQUIRE(rd_sum != nullptr);

    const std::string smspec_ext = fmt_output ? ".FSMSPEC" : ".SMSPEC";
    REQUIRE(fs::exists(rd_sum_get_case(rd_sum.get()) + smspec_ext));
    REQUIRE(fs::exists(std::string(rd_sum_get_abs_path(rd_sum.get())) + "/" +
                       rd_sum_get_base(rd_sum.get()) + smspec_ext));
}

TEST_CASE_METHOD(Tmpdir, "Relative './' prefix produces a normalized case") {
    WriteSpec spec;
    auto case_path = (dirname / "CASE").string();
    write_test_summary(case_path, spec, /*fmt=*/false, /*unified=*/true);

    auto previous_cwd = fs::current_path();
    fs::current_path(dirname);
    auto rd_sum = read_summary("./CASE");
    fs::current_path(previous_cwd);

    REQUIRE(rd_sum != nullptr);
    REQUIRE(std::string(rd_sum_get_base(rd_sum.get())) == "CASE");
    REQUIRE(fs::path(rd_sum_get_abs_path(rd_sum.get())).is_absolute());
    REQUIRE(fs::equivalent(rd_sum_get_abs_path(rd_sum.get()), dirname));
}

TEST_CASE_METHOD(Tmpdir, "rd_sum_get_restart_case is null without restart") {
    WriteSpec spec;
    auto case_path = (dirname / "CASE").string();
    write_test_summary(case_path, spec, /*fmt=*/false, /*unified=*/true);

    auto rd_sum = read_summary(case_path);
    REQUIRE(rd_sum != nullptr);
    REQUIRE(rd_sum_get_restart_case(rd_sum.get()) == nullptr);
    REQUIRE(rd_sum_get_restart_step(rd_sum.get()) <= 0);
}

namespace {
void touch_data_file(const fs::path &path) {
    std::ofstream ofs(path);
    ofs << "RUNSPEC\n";
}
} // namespace

TEST_CASE_METHOD(Tmpdir, "Directory-only to read_summary returns nullptr") {
    SECTION("single .DATA file") {
        auto subdir = dirname / "single_data";
        fs::create_directories(subdir);
        WriteSpec spec;
        write_test_summary((subdir / "CASE").string(), spec, /*fmt=*/false,
                           /*unified=*/true);
        touch_data_file(subdir / "CASE.DATA");
        REQUIRE(read_summary(subdir.string()) == nullptr);
    }
    SECTION("zero .DATA files") {
        auto subdir = dirname / "no_data";
        fs::create_directories(subdir);
        WriteSpec spec;
        write_test_summary((subdir / "CASE").string(), spec, /*fmt=*/false,
                           /*unified=*/true);
        REQUIRE(read_summary(subdir.string()) == nullptr);
    }
    SECTION("multiple .DATA files") {
        auto subdir = dirname / "two_data";
        fs::create_directories(subdir);
        WriteSpec spec;
        write_test_summary((subdir / "CASE1").string(), spec, /*fmt=*/false,
                           /*unified=*/true);
        write_test_summary((subdir / "CASE2").string(), spec, /*fmt=*/false,
                           /*unified=*/true);
        touch_data_file(subdir / "CASE1.DATA");
        touch_data_file(subdir / "CASE2.DATA");
        REQUIRE(read_summary(subdir.string()) == nullptr);
    }
}

TEST_CASE_METHOD(Tmpdir, "Absolute-path input does not get cwd-prefixed") {
    auto abs_case = (fs::absolute(dirname) / "CASE").string();
    WriteSpec spec;
    write_test_summary(abs_case, spec, /*fmt=*/false, /*unified=*/true);

    auto rd_sum = read_summary(abs_case);
    REQUIRE(rd_sum != nullptr);
    REQUIRE(fs::path(rd_sum_get_abs_path(rd_sum.get())).is_absolute());
    REQUIRE(fs::equivalent(rd_sum_get_abs_path(rd_sum.get()),
                           fs::absolute(dirname)));
    REQUIRE(std::string(rd_sum_get_base(rd_sum.get())) == "CASE");
}

TEST_CASE_METHOD(Tmpdir, "input path is normalized") {
    WriteSpec spec;
    auto case_path = (dirname / "CASE").string();
    write_test_summary(case_path, spec, /*fmt=*/false, /*unified=*/true);

    const std::vector<std::string> inputs = {
        dirname.string() + "//CASE",
        dirname.string() + "/./CASE",
    };
    for (const auto &input : inputs) {
        INFO("input=" << input);
        auto rd_sum = read_summary(input);
        REQUIRE(rd_sum != nullptr);
        REQUIRE(std::string(rd_sum_get_base(rd_sum.get())) == "CASE");
        REQUIRE(fs::equivalent(rd_sum_get_abs_path(rd_sum.get()), dirname));
    }
}

TEST_CASE_METHOD(Tmpdir, "Formatted-only case loads via bare base name") {
    WriteSpec spec;
    auto case_path = (dirname / "FCASE").string();
    write_test_summary(case_path, spec, /*fmt=*/true, /*unified=*/true);
    REQUIRE_FALSE(fs::exists(dirname / "FCASE.SMSPEC"));
    REQUIRE(fs::exists(dirname / "FCASE.FSMSPEC"));

    auto rd_sum = read_summary(case_path);
    REQUIRE(rd_sum != nullptr);
    REQUIRE(std::string(rd_sum_get_base(rd_sum.get())) == "FCASE");
    REQUIRE(std::string(rd_sum_get_case(rd_sum.get())) == case_path);
}

TEST_CASE_METHOD(Tmpdir, "write then read produces matching case key") {
    WriteSpec spec;
    auto case_path = (dirname / "RT").string();
    write_test_summary(case_path, spec, /*fmt=*/false, /*unified=*/true);

    auto rd_sum = read_summary(case_path);
    REQUIRE(rd_sum != nullptr);
    REQUIRE(std::string(rd_sum_get_case(rd_sum.get())) == case_path);
    REQUIRE(std::string(rd_sum_get_base(rd_sum.get())) == "RT");
    REQUIRE_THAT(rd_sum_get_last_value_gen_key(rd_sum.get(), "FOPT"),
                 WithinAbs(expected_last_fopt(spec), 1e-3));
}
