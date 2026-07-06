#include <catch2/catch_all.hpp>
#include <catch2/catch_message.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <chrono>
#include <cstdio>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <ios>
#include <optional>
#include <sstream>
#include <memory>
#include <stdexcept>
#include <string>
#include <system_error>
#include <vector>

#include <ert/util/stringlist.hpp>
#include <ert/util/time_t_vector.hpp>
#include <ert/util/util.hpp>

#include <resdata/rd_endian_flip.hpp>
#include <resdata/rd_file.hpp>
#include <resdata/rd_kw.hpp>
#include <resdata/rd_kw_magic.hpp>
#include <resdata/rd_smspec.hpp>
#include <resdata/rd_sum.hpp>
#include <resdata/rd_sum_tstep.hpp>
#include <resdata/rd_sum_vector.hpp>

#include "ert/util/double_vector.hpp"
#include "detail/resdata/rd_unsmry_loader.hpp"
#include "resdata/FortIO.hpp"
#include "resdata/rd_file_view.hpp"
#include "resdata/smspec_node.hpp"
#include "tmpdir.hpp"

using namespace Catch;
using namespace Matchers;

namespace {
struct Chdir {
    fs::path prev;
    explicit Chdir(const fs::path &target) : prev(fs::current_path()) {
        fs::current_path(target);
    }
    ~Chdir() {
        std::error_code ec;
        fs::current_path(prev, ec);
    }
};
} // namespace

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
    auto rd_sum =
        make_summary_writer(case_path, fmt_output, unified, ":",
                            spec.start_time, true, spec.nx, spec.ny, spec.nz);

    const rd::smspec_node *fopt =
        rd_sum_add_var(rd_sum.get(), "FOPT", nullptr, 0, "SM3", 99.0f);
    const rd::smspec_node *bpr =
        rd_sum_add_var(rd_sum.get(), "BPR", nullptr, 567, "BARS", 0.0f);
    const rd::smspec_node *wwct =
        rd_sum_add_var(rd_sum.get(), "WWCT", "OP-1", 0, "(1)", 0.0f);

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

void write_single_string_kw(ERT::FortIO &fortio, const char *name,
                            const char *value) {
    auto kw = make_rd_kw(name, 1, RD_CHAR);
    rd_kw_iset_string8(kw.get(), 0, value);
    rd_kw_fwrite(kw.get(), fortio);
}

void write_single_int_kw(ERT::FortIO &fortio, const char *name, int value) {
    auto kw = make_rd_kw(name, 1, RD_INT);
    rd_kw_iset_int(kw.get(), 0, value);
    rd_kw_fwrite(kw.get(), fortio);
}

void write_malformed_lgr_smspec(const fs::path &path, bool with_lgrs,
                                bool with_numlx, bool with_numly,
                                bool with_numlz) {
    constexpr int n = 1;
    ERT::FortIO fortio(path.string(), std::ios_base::out, false);

    auto dimens = make_rd_kw(DIMENS_KW, DIMENS_SIZE, RD_INT);
    rd_kw_scalar_set_int(dimens.get(), 0);
    rd_kw_iset_int(dimens.get(), DIMENS_SMSPEC_SIZE_INDEX, n);
    rd_kw_iset_int(dimens.get(), DIMENS_SMSPEC_NX_INDEX, 10);
    rd_kw_iset_int(dimens.get(), DIMENS_SMSPEC_NY_INDEX, 10);
    rd_kw_iset_int(dimens.get(), DIMENS_SMSPEC_NZ_INDEX, 10);
    rd_kw_iset_int(dimens.get(), DIMENS_SMSPEC_RESTART_STEP_INDEX, 0);
    rd_kw_fwrite(dimens.get(), fortio);

    auto keywords_kw = make_rd_kw(KEYWORDS_KW, n, RD_CHAR);
    auto wgnames_kw = make_rd_kw(WGNAMES_KW, n, RD_CHAR);
    auto units_kw = make_rd_kw(UNITS_KW, n, RD_CHAR);
    auto nums_kw = make_rd_kw(NUMS_KW, n, RD_INT);

    rd_kw_iset_string8(keywords_kw.get(), 0, "LBPR");
    rd_kw_iset_string8(wgnames_kw.get(), 0, "");
    rd_kw_iset_string8(units_kw.get(), 0, "BARS");
    rd_kw_iset_int(nums_kw.get(), 0, 0);

    rd_kw_fwrite(keywords_kw.get(), fortio);
    rd_kw_fwrite(wgnames_kw.get(), fortio);
    rd_kw_fwrite(nums_kw.get(), fortio);
    rd_kw_fwrite(units_kw.get(), fortio);

    auto startdat = make_rd_kw(STARTDAT_KW, 3, RD_INT);
    rd_kw_iset_int(startdat.get(), STARTDAT_DAY_INDEX, 1);
    rd_kw_iset_int(startdat.get(), STARTDAT_MONTH_INDEX, 1);
    rd_kw_iset_int(startdat.get(), STARTDAT_YEAR_INDEX, 2010);
    rd_kw_fwrite(startdat.get(), fortio);

    if (with_lgrs)
        write_single_string_kw(fortio, LGRS_KW, "LGR1");
    if (with_numlx)
        write_single_int_kw(fortio, NUMLX_KW, 4);
    if (with_numly)
        write_single_int_kw(fortio, NUMLY_KW, 5);
    if (with_numlz)
        write_single_int_kw(fortio, NUMLZ_KW, 6);
}

void expect_smspec_load_throws(const fs::path &header_path,
                               const std::string &msg) {
    REQUIRE_THROWS_WITH(read_smspec(header_path.string(), ":", false),
                        ContainsSubstring(msg));
}

} // namespace

TEST_CASE_METHOD(Tmpdir, "Read summary written by writer") {
    GIVEN("A summary case") {
        const bool fmt_output = GENERATE(true, false);
        const bool unified = GENERATE(true, false);
        const bool lazy = GENERATE(true, false);
        WriteSpec spec;
        auto case_path = (dirname / "CASE").string();
        const time_t end_time =
            write_test_summary(case_path, spec, fmt_output, unified);

        auto rd_sum = read_summary(case_path, ":", lazy);

        SECTION("time range") {
            REQUIRE(rd_sum_get_start_time(rd_sum.get()) == spec.start_time);
            REQUIRE(rd_sum_get_data_start(rd_sum.get()) == spec.start_time);
            REQUIRE(rd_sum_get_end_time(rd_sum.get()) == end_time);
            REQUIRE_THAT(rd_sum_get_first_day(rd_sum.get()),
                         WithinAbs(spec.start_seconds / 86400.0, 1e-9));
            const double last_sim_days =
                (spec.num_report_steps * spec.num_ministep - 1) *
                spec.ministep_length / 86400.0;
            REQUIRE_THAT(rd_sum_get_sim_length(rd_sum.get()),
                         WithinAbs(last_sim_days, 1e-6));
        }

        SECTION("report steps") {
            REQUIRE(rd_sum_get_first_report_step(rd_sum.get()) == 1);
            REQUIRE(rd_sum_get_last_report_step(rd_sum.get()) ==
                    spec.num_report_steps);
            REQUIRE(rd_sum_iget_report_step(rd_sum.get(), 0) == 1);
            REQUIRE(rd_sum_iget_report_step(rd_sum.get(), spec.num_ministep) ==
                    2);
            REQUIRE(rd_sum_iget_report_end(rd_sum.get(), 1) ==
                    spec.num_ministep - 1);
            const double end_of_first_report_days =
                (spec.num_ministep - 1) * spec.ministep_length / 86400.0;
            REQUIRE(rd_sum_get_report_step_from_days(
                        rd_sum.get(), end_of_first_report_days) == 1);
            REQUIRE_THROWS_AS(
                rd_sum_iget_report_end(rd_sum.get(), spec.num_report_steps + 1),
                std::invalid_argument);
            REQUIRE_THROWS_AS(rd_sum_iget_report_step(rd_sum.get(), -1),
                              std::invalid_argument);
            REQUIRE_THROWS_AS(
                rd_sum_iget_report_step(rd_sum.get(), spec.num_report_steps *
                                                          spec.num_ministep),
                std::invalid_argument);
        }

        SECTION("rd_sum_is_instance") {
            REQUIRE(rd_sum_is_instance(rd_sum.get()));
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
            REQUIRE(rd_sum_identify_var_type("FOPT") == RD_SMSPEC_FIELD_VAR);
            REQUIRE(rd_sum_identify_var_type("WWCT") == RD_SMSPEC_WELL_VAR);
            REQUIRE(rd_sum_identify_var_type("BPR") == RD_SMSPEC_BLOCK_VAR);
        }

        THEN("units are stored") {
            REQUIRE(std::string(rd_sum_get_unit(rd_sum.get(), "FOPT")) ==
                    "SM3");
            REQUIRE(std::string(rd_sum_get_unit(rd_sum.get(), "BPR:567")) ==
                    "BARS");
            REQUIRE(std::string(rd_sum_get_unit(rd_sum.get(), "WWCT:OP-1")) ==
                    "(1)");
            REQUIRE(rd_sum_get_unit_system(rd_sum.get()) == RD_METRIC_UNITS);
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
                time_t expected_time = spec.start_time;
                util_inplace_forward_seconds_utc(&expected_time, expected);

                REQUIRE_THAT(rd_sum_get_general_var(rd_sum.get(), i, "FOPT"),
                             WithinAbs(expected, 1e-3));
                REQUIRE_THAT(rd_sum_iget_sim_days(rd_sum.get(), i),
                             WithinAbs(expected / 86400.0, 1e-6));
                REQUIRE_THAT(rd_sum_get_general_var_from_sim_time(
                                 rd_sum.get(), expected_time, "FOPT"),
                             WithinAbs(expected, 1e-3));
                REQUIRE_THAT(rd_sum_get_general_var_from_sim_days(
                                 rd_sum.get(), static_cast<double>(i), "FOPT"),
                             WithinAbs(expected, 1e-3));

                REQUIRE(rd_sum_iget_sim_time(rd_sum.get(), i) == expected_time);
            }
        }

        THEN("missing keys throw out_of_range") {
            REQUIRE_THROWS_AS(
                rd_sum_get_general_var(rd_sum.get(), 0, "NO_SUCH_KEY"),
                std::out_of_range);
            REQUIRE_THROWS_AS(rd_sum_get_general_var_params_index(
                                  rd_sum.get(), "NO_SUCH_KEY"),
                              std::out_of_range);
            REQUIRE_THROWS_AS(
                rd_sum_get_general_var_node(rd_sum.get(), "NO_SUCH_KEY"),
                std::out_of_range);
        }

        THEN("iget with out-of-range internal index throws") {
            const int n = rd_sum_get_data_length(rd_sum.get());
            REQUIRE_THROWS_AS(rd_sum_iget_sim_time(rd_sum.get(), -1),
                              std::invalid_argument);
            REQUIRE_THROWS_AS(rd_sum_iget_sim_time(rd_sum.get(), n),
                              std::invalid_argument);
            REQUIRE_THROWS_AS(rd_sum_iget_sim_days(rd_sum.get(), n),
                              std::invalid_argument);
        }

        THEN("first_gt returns the first index exceeding the limit") {
            const int fopt_idx =
                rd_sum_get_general_var_params_index(rd_sum.get(), "FOPT");

            REQUIRE(rd_sum_get_first_gt(rd_sum.get(), fopt_idx, -1.0) == 0);
            REQUIRE(rd_sum_get_first_gt(rd_sum.get(), fopt_idx, 0.0) == 1);
            REQUIRE(rd_sum_get_first_gt(rd_sum.get(), fopt_idx,
                                        spec.ministep_length) == 2);
            REQUIRE(rd_sum_get_first_gt(rd_sum.get(), fopt_idx,
                                        2.5 * spec.ministep_length) == 3);

            const double last_sim_seconds =
                (spec.num_report_steps * spec.num_ministep - 1) *
                spec.ministep_length;
            REQUIRE(rd_sum_get_first_gt(rd_sum.get(), fopt_idx,
                                        last_sim_seconds) == -1);

            const int bpr_idx =
                rd_sum_get_general_var_params_index(rd_sum.get(), "BPR:567");
            REQUIRE(rd_sum_get_first_gt(rd_sum.get(), bpr_idx,
                                        10.0 * spec.ministep_length) == 2);
        }

        THEN("first_lt returns the first index below the limit") {
            const int fopt_idx =
                rd_sum_get_general_var_params_index(rd_sum.get(), "FOPT");

            REQUIRE(rd_sum_get_first_lt(rd_sum.get(), fopt_idx, 1.0) == 0);
            REQUIRE(rd_sum_get_first_lt(rd_sum.get(), fopt_idx, 0.0) == -1);
            REQUIRE(rd_sum_get_first_lt(rd_sum.get(), fopt_idx, -1.0) == -1);

            const int wwct_idx =
                rd_sum_get_general_var_params_index(rd_sum.get(), "WWCT:OP-1");
            REQUIRE(rd_sum_get_first_lt(rd_sum.get(), wwct_idx, 1.0) == 0);
        }

        THEN("matching general var list collects keys") {
            auto list = stringlist_ptr(
                rd_sum_alloc_matching_general_var_list(rd_sum.get(), "F*"),
                &stringlist_free);
            REQUIRE(stringlist_get_size(list.get()) == 1);
            REQUIRE(std::string(stringlist_iget(list.get(), 0)) == "FOPT");
        }

        THEN("select_matching_general_var_list appends new matches") {
            auto keys = make_stringlist();

            rd_sum_select_matching_general_var_list(rd_sum.get(), "F*",
                                                    keys.get());
            REQUIRE(stringlist_get_size(keys.get()) == 1);
            REQUIRE(std::string(stringlist_iget(keys.get(), 0)) == "FOPT");

            rd_sum_select_matching_general_var_list(rd_sum.get(), "W*",
                                                    keys.get());
            REQUIRE(stringlist_get_size(keys.get()) == 2);
            REQUIRE(stringlist_contains(keys.get(), "FOPT"));
            REQUIRE(stringlist_contains(keys.get(), "WWCT:OP-1"));
        }

        THEN("well list returns the producer") {
            auto wells =
                stringlist_ptr(rd_sum_alloc_well_list(rd_sum.get(), nullptr),
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
        THEN("get_general_var_from_sim_time with out-of-range time throws") {
            time_t before = spec.start_time;
            util_inplace_forward_seconds_utc(&before, -spec.ministep_length);
            REQUIRE_THROWS_AS(rd_sum_get_general_var_from_sim_time(
                                  rd_sum.get(), before, "FOPT"),
                              std::out_of_range);
            time_t after = end_time;
            util_inplace_forward_seconds_utc(&after, spec.ministep_length);
            REQUIRE_THROWS_AS(rd_sum_get_general_var_from_sim_time(
                                  rd_sum.get(), after, "FOPT"),
                              std::out_of_range);
        }
        THEN("Path accessors point at the written case") {
            REQUIRE(std::string(rd_sum_get_base(rd_sum.get())) == "CASE");
            REQUIRE(rd_sum_get_restart_case(rd_sum.get()) == nullptr);
        }

        THEN("get_restart_step is <= 0 if there is no restart") {
            REQUIRE(rd_sum_get_restart_step(rd_sum.get()) <= 0);
        }

        SECTION("Allocated time and data vectors") {
            time_t_vector_ptr times(
                rd_sum_alloc_time_vector(rd_sum.get(), false),
                &time_t_vector_free);
            REQUIRE(time_t_vector_size(times.get()) ==
                    rd_sum_get_data_length(rd_sum.get()));
            REQUIRE(time_t_vector_iget(times.get(), 0) == spec.start_time);

            const int param_index =
                rd_sum_get_general_var_params_index(rd_sum.get(), "FOPT");
            auto data = double_vector_ptr(
                rd_sum_alloc_data_vector(rd_sum.get(), param_index, false),
                &double_vector_free);
            REQUIRE(double_vector_size(data.get()) ==
                    rd_sum_get_data_length(rd_sum.get()));
            REQUIRE_THAT(double_vector_iget(data.get(), 0),
                         WithinAbs(0.0, 1e-6));

            REQUIRE_THROWS_AS(
                rd_sum_alloc_data_vector(rd_sum.get(), 9999, false),
                std::out_of_range);
        }

        THEN("rd_smspec_equal is reflexive on the loaded smspec") {
            const rd_smspec_type *smspec = rd_sum_get_smspec(rd_sum.get());
            REQUIRE(rd_smspec_equal(smspec, smspec));
        }

        WHEN("calling rd_sum_alloc_time_solution") {
            auto solve = [&](const char *key, double cmp,
                             bool clamp_lower = false) {
                return time_t_vector_ptr(
                    rd_sum_alloc_time_solution(rd_sum.get(), key, cmp,
                                               clamp_lower),
                    &time_t_vector_free);
            };

            const double day = spec.ministep_length;

            THEN("Solving for monotonic gives single crossing") {
                auto sol = solve("FOPT", 1.5 * day);
                REQUIRE(time_t_vector_size(sol.get()) == 1);
                time_t expected = spec.start_time;
                util_inplace_forward_seconds_utc(&expected, 1.5 * day);
                REQUIRE(time_t_vector_iget(sol.get(), 0) == expected);
            }

            THEN("Solving for exact sample returns that value") {
                auto sol = solve("FOPT", 2.0 * day);
                REQUIRE(time_t_vector_size(sol.get()) == 1);
                REQUIRE(time_t_vector_iget(sol.get(), 0) ==
                        rd_sum_iget_sim_time(rd_sum.get(), 2));
            }

            THEN("value below the series range yields empty") {
                auto sol = solve("FOPT", -1.0);
                REQUIRE(sol.get() != nullptr);
                REQUIRE(time_t_vector_size(sol.get()) == 0);
            }

            THEN("value above the series range yields empty") {
                const double last =
                    (spec.num_report_steps * spec.num_ministep - 1) * day;
                auto sol = solve("FOPT", 10.0 * last);
                REQUIRE(sol.get() != nullptr);
                REQUIRE(time_t_vector_size(sol.get()) == 0);
            }

            THEN("resolution uses the requested key") {
                auto fopt_sol = solve("FOPT", 1.5 * day);
                auto bpr_sol = solve("BPR:567", 10.0 * 1.5 * day);
                REQUIRE(time_t_vector_size(bpr_sol.get()) == 1);
                REQUIRE(time_t_vector_iget(bpr_sol.get(), 0) ==
                        time_t_vector_iget(fopt_sol.get(), 0));
            }

            THEN("rate variable with rates_clamp_lower=true picks the lower "
                 "edge") {
                auto sol = solve("WWCT:OP-1", 100.0 * 1.5 * day,
                                 /*clamp_lower=*/true);
                REQUIRE(time_t_vector_size(sol.get()) == 1);
                time_t expected = spec.start_time;
                util_inplace_forward_seconds_utc(&expected, day + 1);
                REQUIRE(time_t_vector_iget(sol.get(), 0) == expected);
            }

            THEN("rate variable with rates_clamp_lower=false picks the upper "
                 "edge") {
                auto sol = solve("WWCT:OP-1", 100.0 * 1.5 * day,
                                 /*clamp_lower=*/false);
                REQUIRE(time_t_vector_size(sol.get()) == 1);
                REQUIRE(time_t_vector_iget(sol.get(), 0) ==
                        rd_sum_iget_sim_time(rd_sum.get(), 2));
            }

            THEN("unknown key throws std::out_of_range") {
                REQUIRE_THROWS_AS(rd_sum_alloc_time_solution(
                                      rd_sum.get(), "NO_SUCH_KEY", 0.0, false),
                                  std::out_of_range);
            }
        }
    }
}

TEST_CASE_METHOD(Tmpdir, "rd_sum_add_tstep rejects negative report steps") {
    const auto case_path = (dirname / "NEGATIVE_REPORT_STEP").string();
    auto rd_sum = make_summary_writer(case_path, /*fmt_output=*/false,
                                      /*unified=*/true, ":",
                                      util_make_date_utc(1, 1, 2010),
                                      /*time_in_days=*/true, 10, 11, 12);

    REQUIRE_THROWS_AS(rd_sum_add_tstep(rd_sum.get(), -1, 0.0),
                      std::invalid_argument);
}

TEST_CASE_METHOD(Tmpdir,
                 "rd_sum_alloc_time_solution finds multiple crossings on a "
                 "non-monotonic series") {
    const auto case_path = (dirname / "NMCASE").string();
    const time_t start_time = util_make_date_utc(1, 1, 2010);
    const double dt = 86400.0;
    const std::vector<double> samples{0.0, 1.0, 2.0, 3.0, 2.0, 1.0, 0.0};

    {
        auto rd_sum = make_summary_writer(case_path, /*fmt=*/false,
                                          /*unified=*/true, ":", start_time,
                                          true, 10, 10, 10);
        const rd::smspec_node *bpr =
            rd_sum_add_var(rd_sum.get(), "BPR", nullptr, 567, "BARS", 0.0f);

        double sim_seconds = 0.0;
        for (std::size_t i = 0; i < samples.size(); ++i) {
            rd_sum_tstep_type *tstep = rd_sum_add_tstep(
                rd_sum.get(), static_cast<int>(i) + 1, sim_seconds);
            rd_sum_tstep_set_from_node(tstep, *bpr, samples[i]);
            sim_seconds += dt;
        }
        rd_sum_fwrite(rd_sum.get());
    }

    auto rd_sum = read_summary(case_path);

    time_t_vector_ptr sol(
        rd_sum_alloc_time_solution(rd_sum.get(), "BPR:567", 1.5,
                                   /*rates_clamp_lower=*/false),
        &time_t_vector_free);

    REQUIRE(time_t_vector_size(sol.get()) == 2);

    time_t rising = start_time;
    util_inplace_forward_seconds_utc(&rising, 1.5 * dt);
    time_t falling = start_time;
    util_inplace_forward_seconds_utc(&falling, 4.5 * dt);

    REQUIRE(time_t_vector_iget(sol.get(), 0) == rising);
    REQUIRE(time_t_vector_iget(sol.get(), 1) == falling);
    REQUIRE(time_t_vector_iget(sol.get(), 0) <
            time_t_vector_iget(sol.get(), 1));
}

TEST_CASE_METHOD(Tmpdir, "rd_sum_add_local_var registers LGR smspec nodes") {
    GIVEN("A writer with local variables added") {
        const auto case_path = (dirname / "LOCALCASE").string();
        const time_t start_time = util_make_date_utc(1, 1, 2010);

        auto rd_sum = make_summary_writer(case_path, /*fmt=*/false,
                                          /*unified=*/true, ":", start_time,
                                          true, 10, 10, 10);

        const rd::smspec_node *lwell = rd_sum_add_local_var(
            rd_sum.get(), "LWOPR", "OP-1", 0, "SM3/DAY", "LGR1", 1, 2, 3, 0.0f);
        const rd::smspec_node *lblock = rd_sum_add_local_var(
            rd_sum.get(), "LBPR", "", 0, "BARS", "LGR1", 4, 5, 6, 0.0f);
        const rd::smspec_node *lcompl = rd_sum_add_local_var(
            rd_sum.get(), "LCPR", "OP-1", 0, "SM3/DAY", "LGR1", 7, 8, 9, 0.0f);

        THEN("registers local well, block and completion variables") {
            REQUIRE(lwell != nullptr);
            REQUIRE(lblock != nullptr);
            REQUIRE(lcompl != nullptr);

            REQUIRE(std::string(lwell->get_keyword()) == "LWOPR");
            REQUIRE(std::string(lwell->get_lgr_name()) == "LGR1");
            REQUIRE(std::string(lwell->get_wgname()) == "OP-1");

            REQUIRE(std::string(lblock->get_keyword()) == "LBPR");
            REQUIRE(std::string(lblock->get_lgr_name()) == "LGR1");
            REQUIRE(lblock->get_lgr_ijk()[0] == 4);
            REQUIRE(lblock->get_lgr_ijk()[1] == 5);
            REQUIRE(lblock->get_lgr_ijk()[2] == 6);

            REQUIRE(std::string(lcompl->get_keyword()) == "LCPR");
            REQUIRE(std::string(lcompl->get_lgr_name()) == "LGR1");
            REQUIRE(std::string(lcompl->get_wgname()) == "OP-1");
            REQUIRE(lcompl->get_lgr_ijk()[0] == 7);
            REQUIRE(lcompl->get_lgr_ijk()[1] == 8);
            REQUIRE(lcompl->get_lgr_ijk()[2] == 9);
        }

        THEN("generated gen_key follows the LOCAL_* format") {
            REQUIRE(std::string(lwell->get_gen_key1()) == "LWOPR:LGR1:OP-1");
            REQUIRE(std::string(lblock->get_gen_key1()) == "LBPR:LGR1:4,5,6");
            REQUIRE(std::string(lcompl->get_gen_key1()) ==
                    "LCPR:LGR1:OP-1:7,8,9");
        }

        THEN("adding a local var after a timestep has been written throws") {
            auto rd_sum = make_summary_writer(case_path, /*fmt=*/false,
                                              /*unified=*/true, ":", start_time,
                                              true, 10, 10, 10);

            const rd::smspec_node *fopt =
                rd_sum_add_var(rd_sum.get(), "FOPT", nullptr, 0, "SM3", 0.0f);
            rd_sum_tstep_type *tstep = rd_sum_add_tstep(rd_sum.get(), 1, 0.0);
            rd_sum_tstep_set_from_node(tstep, *fopt, 0.0);

            REQUIRE_THROWS_AS(rd_sum_add_local_var(rd_sum.get(), "LWOPR",
                                                   "OP-1", 0, "SM3/DAY", "LGR1",
                                                   1, 2, 3, 0.0f),
                              std::invalid_argument);
            REQUIRE_THROWS_AS(rd_sum_add_var(rd_sum.get(), "WOPR", "OP-1", 0,
                                             "SM3/DAY", 0.0f),
                              std::invalid_argument);
        }
    }
}

TEST_CASE_METHOD(Tmpdir, "rd_sum_alloc_group_list returns group names") {
    const auto case_path = (dirname / "GCASE").string();

    {
        auto rd_sum = make_summary_writer(case_path, /*fmt_output=*/false,
                                          /*unified=*/true, ":",
                                          util_make_date_utc(1, 1, 2010), true,
                                          10, 10, 10);

        const rd::smspec_node *gopr_g1 =
            rd_sum_add_var(rd_sum.get(), "GOPR", "G1", 0, "SM3/DAY", 0.0f);
        const rd::smspec_node *gopt_g1 =
            rd_sum_add_var(rd_sum.get(), "GOPT", "G1", 0, "SM3", 0.0f);
        const rd::smspec_node *gwct_g2 =
            rd_sum_add_var(rd_sum.get(), "GWCT", "G2", 0, "(1)", 0.0f);
        const rd::smspec_node *ggpr_north =
            rd_sum_add_var(rd_sum.get(), "GGPR", "NORTH", 0, "SM3/DAY", 0.0f);
        const rd::smspec_node *wopr_op1 =
            rd_sum_add_var(rd_sum.get(), "WOPR", "OP-1", 0, "SM3/DAY", 0.0f);

        rd_sum_tstep_type *tstep = rd_sum_add_tstep(rd_sum.get(), 1, 0.0);
        rd_sum_tstep_set_from_node(tstep, *gopr_g1, 0.0);
        rd_sum_tstep_set_from_node(tstep, *gopt_g1, 0.0);
        rd_sum_tstep_set_from_node(tstep, *gwct_g2, 0.0);
        rd_sum_tstep_set_from_node(tstep, *ggpr_north, 0.0);
        rd_sum_tstep_set_from_node(tstep, *wopr_op1, 0.0);
        rd_sum_fwrite(rd_sum.get());
    }

    auto rd_sum = read_summary(case_path);

    SECTION("null pattern enumerates all unique groups") {
        auto groups = stringlist_ptr(
            rd_sum_alloc_group_list(rd_sum.get(), nullptr), &stringlist_free);
        REQUIRE(stringlist_get_size(groups.get()) == 3);
        REQUIRE(stringlist_contains(groups.get(), "G1"));
        REQUIRE(stringlist_contains(groups.get(), "G2"));
        REQUIRE(stringlist_contains(groups.get(), "NORTH"));
    }

    SECTION("exact pattern matches a single group") {
        auto groups = stringlist_ptr(
            rd_sum_alloc_group_list(rd_sum.get(), "G1"), &stringlist_free);
        REQUIRE(stringlist_get_size(groups.get()) == 1);
        REQUIRE(std::string(stringlist_iget(groups.get(), 0)) == "G1");
    }

    SECTION("glob pattern matches a subset of groups") {
        auto groups = stringlist_ptr(
            rd_sum_alloc_group_list(rd_sum.get(), "G*"), &stringlist_free);
        REQUIRE(stringlist_get_size(groups.get()) == 2);
        REQUIRE(stringlist_contains(groups.get(), "G1"));
        REQUIRE(stringlist_contains(groups.get(), "G2"));
        REQUIRE_FALSE(stringlist_contains(groups.get(), "NORTH"));
    }

    SECTION("non-matching pattern returns an empty list") {
        auto groups = stringlist_ptr(
            rd_sum_alloc_group_list(rd_sum.get(), "DOES_NOT_EXIST"),
            &stringlist_free);
        REQUIRE(groups.get() != nullptr);
        REQUIRE(stringlist_get_size(groups.get()) == 0);
    }

    SECTION("wildcard pattern returns only group names, never well names") {
        auto groups = stringlist_ptr(rd_sum_alloc_group_list(rd_sum.get(), "*"),
                                     &stringlist_free);
        REQUIRE(stringlist_get_size(groups.get()) == 3);
        REQUIRE_FALSE(stringlist_contains(groups.get(), "OP-1"));
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
        auto restart_sum =
            make_summary_writer(restart_path,
                                /*fmt_output=*/false,
                                /*unified=*/true, ":", spec.start_time, true,
                                spec.nx, spec.ny, spec.nz, base_path);

        REQUIRE(rd_sum_can_write(restart_sum.get()));

        const rd::smspec_node *fopt =
            rd_sum_add_var(restart_sum.get(), "FOPT", nullptr, 0, "SM3", 99.0f);
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
    Chdir cd{dirname};
    WriteSpec spec;
    const std::string base_name = "CASE1";
    const std::string restart_name = "CASE2";
    write_test_summary(base_name, spec, fmt_output, unified);

    {
        auto restart_sum = make_summary_writer(
            restart_name, fmt_output, unified, ":", spec.start_time, true,
            spec.nx, spec.ny, spec.nz, base_name);
        rd_smspec_type *smspec = rd_sum_get_smspec(restart_sum.get());
        REQUIRE(rd_smspec_get_params_size(smspec) == 1);
        const rd::smspec_node *fopt =
            rd_sum_add_var(restart_sum.get(), "FOPT", nullptr, 0, "SM3", 99.0f);
        rd_sum_add_var(restart_sum.get(), "BPR", nullptr, 567, "BARS", 0.0f);
        rd_sum_add_var(restart_sum.get(), "WWCT", "OP-1", 0, "(1)", 0.0f);
        REQUIRE(rd_smspec_get_params_size(smspec) == 4);
        rd_sum_tstep_type *tstep = rd_sum_add_tstep(restart_sum.get(), 1, 0.0);
        rd_sum_tstep_set_from_node(tstep, *fopt, 0.0);
        REQUIRE(rd_sum_tstep_get_from_node(tstep, *fopt) == 0.0);
        rd_sum_fwrite(restart_sum.get());
    }

    const std::string smspec_ext = fmt_output ? ".FSMSPEC" : ".SMSPEC";
    auto restart_file = open_rd_file(restart_name + smspec_ext, 0);
    REQUIRE(restart_file != nullptr);
    rd_file_view_type *view = rd_file_get_global_view(restart_file.get());
    REQUIRE(rd_file_view_has_kw(view, RESTART_KW));

    SECTION("Parent case name is padded across 8-char blocks") {
        rd_kw_type *restart_kw =
            rd_file_view_iget_named_kw(view, RESTART_KW, 0);
        REQUIRE(rd_kw_get_size(restart_kw) == 8);
        REQUIRE(std::string(static_cast<const char *>(
                    rd_kw_iget_ptr(restart_kw, 0))) == "CASE1   ");
        REQUIRE(std::string(static_cast<const char *>(
                    rd_kw_iget_ptr(restart_kw, 1))) == "        ");
    }

    SECTION("Loading with include_restart inserts the parent's FOPT values") {
        auto restart_sum = read_summary(restart_name);
        auto base_sum = read_summary(base_name);
        REQUIRE(restart_sum != nullptr);
        REQUIRE(base_sum != nullptr);
        const int n = rd_sum_get_data_length(base_sum.get());
        for (int i = 0; i < n; ++i) {
            REQUIRE(rd_sum_get_general_var(base_sum.get(), i, "FOPT") ==
                    rd_sum_get_general_var(restart_sum.get(), i, "FOPT"));
        }
    }
}

TEST_CASE_METHOD(Tmpdir, "Restart case names are split across the 8 blocks") {
    Chdir cd{dirname};
    std::string restart_case;
    for (int n = 0; n < 8; ++n)
        restart_case += "WWWWGGG" + std::to_string(n);
    REQUIRE(restart_case.size() == 64);

    const std::string name = "THE_CASE";
    const time_t start_time = util_make_date_utc(1, 1, 2010);
    {
        auto rd_sum = make_summary_writer(name, false, true, ":", start_time,
                                          true, 3, 3, 3, restart_case, 77);
        rd_sum_fwrite(rd_sum.get());
    }

    auto smspec_file = open_rd_file(name + ".SMSPEC", 0);
    rd_file_view_type *view = rd_file_get_global_view(smspec_file.get());
    REQUIRE(rd_file_view_has_kw(view, RESTART_KW));
    rd_kw_type *restart_kw = rd_file_view_iget_named_kw(view, RESTART_KW, 0);
    REQUIRE(rd_kw_get_size(restart_kw) == 8);
    for (int n = 0; n < 8; ++n) {
        const std::string expected = "WWWWGGG" + std::to_string(n);
        REQUIRE(std::string(rd_kw_iget_char_ptr(restart_kw, n)) == expected);
    }
}

TEST_CASE_METHOD(Tmpdir, "Restart names >64 characters are ignored") {
    const time_t start_time = util_make_date_utc(1, 1, 2010);
    const std::string too_long(72, 'A');
    auto smspec = std::unique_ptr<rd_smspec_type, decltype(&rd_smspec_free)>(
        rd_smspec_alloc_restart_writer(":", too_long.c_str(), 10, start_time,
                                       true, 3, 3, 3),
        &rd_smspec_free);
    REQUIRE(rd_smspec_get_restart_case(smspec.get()) == nullptr);
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
        auto rd_sum = read_summary(case_path + ".FSMSPEC", ":");
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
        auto rd_sum = read_summary(case_path, ":");
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
        Chdir cd{subdir};
        auto rd_sum = read_summary("CASE");

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

TEST_CASE_METHOD(Tmpdir, "Malformed SMSPEC LGR metadata is rejected") {
    SECTION("LGR variable without LGRS/NUML* is rejected") {
        const fs::path header_path = dirname / "MISSING_LGR_META.SMSPEC";
        write_malformed_lgr_smspec(header_path, false, false, false, false);

        expect_smspec_load_throws(header_path,
                                  "required LGR metadata keywords are missing");
    }

    SECTION("LGRS without complete NUMLX/NUMLY/NUMLZ is rejected") {
        const fs::path header_path = dirname / "MISSING_NUMLZ.SMSPEC";
        write_malformed_lgr_smspec(header_path, true, true, true, false);

        expect_smspec_load_throws(
            header_path,
            "SMSPEC header has LGRS keyword but is missing one or more "
            "required LGR index keywords");
    }
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

    Chdir cd{dirname};
    auto rd_sum = read_summary("./CASE");

    REQUIRE(rd_sum != nullptr);
    REQUIRE(std::string(rd_sum_get_base(rd_sum.get())) == "CASE");
    REQUIRE(fs::path(rd_sum_get_abs_path(rd_sum.get())).is_absolute());
    REQUIRE(fs::equivalent(rd_sum_get_abs_path(rd_sum.get()), dirname));
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

SCENARIO_METHOD(Tmpdir, "Loading Restarts") {
    GIVEN("A chain CASE1 -> CASE2 -> CASE3 of summary cases with BPR:1-3") {
        const time_t start_time = util_make_date_utc(1, 1, 2010);
        const double ministep_length = 86400.0;            // 1 day in seconds
        const double step_spacing = 3.0 * ministep_length; // 3 days per step
        const int nx = 10, ny = 10, nz = 10;

        const std::vector<std::vector<double>> case1_bpr = {
            {100.0, 200.0, 300.0},
            {110.0, 210.0, 310.0},
            {120.0, 220.0, 320.0},
        };
        const std::vector<std::vector<double>> case2_bpr = {
            {1000.0, 2000.0, 3000.0, 4000.0},
            {1100.0, 2100.0, 3100.0, 4100.0},
        };
        const std::vector<std::vector<double>> case3_bpr = {
            {10000.0, 20000.0, 30000.0, 40000.0},
            {11000.0, 21000.0, 31000.0, 41000.0},
            {12000.0, 22000.0, 32000.0, 42000.0},
        };

        // When loaded with include_restart we should get the "total":
        const std::vector<std::vector<double>> total_case2_bpr = {
            {100, 200, 300, 1000, 2000, 3000, 4000},
            {110, 210, 310, 1100, 2100, 3100, 4100},
        };
        // In the "total" of CASE3, BPR:3 is absent from CASE2 (its restart),
        // so those positions are filled with PARAMS_GLOBAL_DEFAULT (-99)
        constexpr double missing = -99.0;
        const std::vector<std::vector<double>> total_case3_bpr = {
            {100, 200, 300, 1000, 2000, 10000, 20000, 30000, 40000},
            {110, 210, 310, 1100, 2100, 11000, 21000, 31000, 41000},
            {120, 220, 320, missing, missing, 12000, 22000, 32000, 42000},
        };

        auto write_case = [&](const std::string &path,
                              const std::optional<std::string> &restart_from,
                              double start_seconds, int report_step_offset,
                              const std::vector<std::vector<double>> &bpr_data,
                              const std::vector<int> &bpr_cells) {
            rd_sum_ptr sum =
                make_summary_writer(path, false, true, ":", start_time, true,
                                    nx, ny, nz, restart_from);
            std::vector<const rd::smspec_node *> nodes;
            for (int cell : bpr_cells)
                nodes.push_back(rd_sum_add_var(sum.get(), "BPR", nullptr, cell,
                                               "BARS", 0.0f));

            const int num_steps =
                bpr_data.empty() ? 0 : int(bpr_data[0].size());
            double sim_seconds = start_seconds;
            for (int j = 0; j < num_steps; ++j) {
                rd_sum_tstep_type *tstep = rd_sum_add_tstep(
                    sum.get(), report_step_offset + j + 1, sim_seconds);
                for (size_t k = 0; k < nodes.size(); ++k)
                    rd_sum_tstep_set_from_node(tstep, *nodes[k],
                                               bpr_data[k][j]);
                sim_seconds += step_spacing;
            }
            rd_sum_fwrite(sum.get());
        };

        const auto case1_path = (dirname / "CASE1").string();
        const auto case2_path = (dirname / "CASE2").string();
        const auto case3_path = (dirname / "CASE3").string();
        const auto case4_path = (dirname / "CASE4").string();

        write_case(case1_path, std::nullopt, 0.0, 0, case1_bpr, {1, 2, 3});
        // CASE2 starts at 7.5 days, after CASE1's last step (6 days).
        write_case(case2_path, case1_path, 2.5 * step_spacing, 3, case2_bpr,
                   {1, 2});
        // CASE3 starts at 12 days, after CASE2's step at 10.5 days.
        write_case(case3_path, case2_path, 4.0 * step_spacing, 5, case3_bpr,
                   {1, 2, 3});

        auto check_vector = [](rd_sum_type *sum, const std::string &key,
                               const std::vector<double> &expected) {
            const int idx =
                rd_sum_get_general_var_params_index(sum, key.c_str());
            double_vector_ptr data(rd_sum_alloc_data_vector(sum, idx, false),
                                   &double_vector_free);
            REQUIRE(static_cast<size_t>(double_vector_size(data.get())) ==
                    expected.size());
            for (size_t j = 0; j < expected.size(); ++j)
                REQUIRE_THAT(
                    double_vector_iget(data.get(), static_cast<int>(j)),
                    WithinAbs(expected[j], 1e-6));
        };

        WHEN("All parent SMSPEC files are present") {
            THEN("rd_smspec_equal is false for cases with different BPR") {
                auto sum1 = read_summary(case1_path, ":", true,
                                         /*include_restart=*/false);
                auto sum2 = read_summary(case2_path, ":", true,
                                         /*include_restart=*/false);
                auto sum3 = read_summary(case3_path, ":", true,
                                         /*include_restart=*/false);
                const rd_smspec_type *s1 = rd_sum_get_smspec(sum1.get());
                const rd_smspec_type *s2 = rd_sum_get_smspec(sum2.get());
                const rd_smspec_type *s3 = rd_sum_get_smspec(sum3.get());

                REQUIRE_FALSE(rd_smspec_equal(s1, s2));
                REQUIRE_FALSE(rd_smspec_equal(s2, s1));
                REQUIRE_FALSE(rd_smspec_equal(s2, s3));
                REQUIRE_FALSE(rd_smspec_equal(s3, s2));

                REQUIRE(rd_smspec_equal(s1, s3));
                REQUIRE(rd_smspec_equal(s3, s1));
            }

            THEN("Loading CASE1 returns its own data only") {
                auto sum = read_summary(case1_path);

                check_vector(sum.get(), "BPR:1", case1_bpr[0]);
                check_vector(sum.get(), "BPR:2", case1_bpr[1]);
                check_vector(sum.get(), "BPR:3", case1_bpr[2]);
            }

            THEN("Loading CASE2 yields the combined Total CASE2 series") {
                auto sum = read_summary(case2_path);
                check_vector(sum.get(), "BPR:1", total_case2_bpr[0]);
                check_vector(sum.get(), "BPR:2", total_case2_bpr[1]);
            }

            THEN("Loading CASE3 yields the combined Total CASE3 series") {
                auto sum = read_summary(case3_path);

                check_vector(sum.get(), "BPR:1", total_case3_bpr[0]);
                check_vector(sum.get(), "BPR:2", total_case3_bpr[1]);
                check_vector(sum.get(), "BPR:3", total_case3_bpr[2]);
            }
        }

        WHEN("CASE2 is loaded without including its restart") {
            auto sum = read_summary(case2_path, ":", /*lazy_load=*/true,
                                    /*include_restart=*/false);
            THEN("Only its own data points are returned") {
                check_vector(sum.get(), "BPR:1", case2_bpr[0]);
                check_vector(sum.get(), "BPR:2", case2_bpr[1]);
                REQUIRE(rd_sum_get_data_length(sum.get()) ==
                        int(case2_bpr[0].size()));
            }
        }

        WHEN("CASE3 is loaded without including its restart") {
            auto sum = read_summary(case3_path, ":", /*lazy_load=*/true,
                                    /*include_restart=*/false);
            THEN("Only its own data points are returned") {
                check_vector(sum.get(), "BPR:1", case3_bpr[0]);
                check_vector(sum.get(), "BPR:2", case3_bpr[1]);
                check_vector(sum.get(), "BPR:3", case3_bpr[2]);
                REQUIRE(rd_sum_get_data_length(sum.get()) ==
                        int(case3_bpr[0].size()));
            }
        }

        WHEN("The CASE1 SMSPEC file is removed") {
            REQUIRE(fs::remove(case1_path + ".SMSPEC"));

            THEN("Loading CASE2 returns only its own data") {
                auto sum = read_summary(case2_path);
                check_vector(sum.get(), "BPR:1", case2_bpr[0]);
                check_vector(sum.get(), "BPR:2", case2_bpr[1]);
                REQUIRE(rd_sum_get_restart_case(sum.get()) == nullptr);
            }
        }
        WHEN("CASE3 is post-processed into CASE4 by appending a duplicate "
             "BPR keyword and a WTPRWI1 placeholder") {
            auto smspec_in = open_rd_file(case3_path + ".SMSPEC", 0);
            auto sum_in = open_rd_file(case3_path + ".UNSMRY", 0);

            rd_kw_type *keywords =
                rd_file_iget_named_kw(smspec_in.get(), "KEYWORDS", 0);
            rd_kw_resize(keywords, 5);
            rd_kw_iset_char_ptr(keywords, 3, "WTPRWI1");
            rd_kw_iset_char_ptr(keywords, 4, "BPR");

            rd_kw_type *nums =
                rd_file_iget_named_kw(smspec_in.get(), "NUMS", 0);
            rd_kw_resize(nums, 5);
            int *nums_ptr = rd_kw_get_int_ptr(nums);
            nums_ptr[3] = 5;
            nums_ptr[4] = 8;

            rd_kw_type *wgnames =
                rd_file_iget_named_kw(smspec_in.get(), "WGNAMES", 0);
            rd_kw_resize(wgnames, 5);
            rd_kw_iset_char_ptr(wgnames, 4, ":+:+:+:+");

            rd_kw_type *units =
                rd_file_iget_named_kw(smspec_in.get(), "UNITS", 0);
            rd_kw_resize(units, 5);
            rd_kw_iset_char_ptr(units, 4, "BARS");

            const int num_params =
                rd_file_get_num_named_kw(sum_in.get(), "PARAMS");
            for (int i = 0; i < num_params; ++i) {
                rd_kw_type *params_kw =
                    rd_file_iget_named_kw(sum_in.get(), "PARAMS", i);
                rd_kw_resize(params_kw, 5);
                auto *ptr = static_cast<float *>(rd_kw_get_void_ptr(params_kw));
                ptr[4] = ptr[3];
                ptr[3] = -1.0f;
            }

            {
                ERT::FortIO out(case4_path + ".UNSMRY", std::ios_base::out,
                                false);
                rd_file_fwrite_fortio(sum_in.get(), out, 0);
            }
            {
                ERT::FortIO out(case4_path + ".SMSPEC", std::ios_base::out,
                                false);
                rd_file_fwrite_fortio(smspec_in.get(), out, 0);
            }

            THEN("The duplicate BPR:8 key is copied from BPR:3 "
                 "for CASE3's steps and missing for the restart range") {
                auto sum = read_summary(case4_path);
                REQUIRE(rd_sum_has_key(sum.get(), "BPR:8"));

                // 3 (CASE1) + 2 (CASE2) + 4 (CASE3) = 9 ministeps; the first
                // 5 are filled with the default (the restart chain doesn't
                // contain BPR:8), the last 4 are the BPR:3 values copied into
                // PARAMS[4] when CASE4 was synthesised.
                check_vector(sum.get(), "BPR:8",
                             {
                                 missing,
                                 missing,
                                 missing,
                                 missing,
                                 missing,
                                 12000.0,
                                 22000.0,
                                 32000.0,
                                 42000.0,
                             });
            }

            THEN("The BPR:1-2 keys contain the merged series") {
                auto sum = read_summary(case4_path);

                check_vector(sum.get(), "BPR:1", total_case3_bpr[0]);
                check_vector(sum.get(), "BPR:2", total_case3_bpr[1]);
            }
        }
    }
}

SCENARIO_METHOD(Tmpdir, "rd::unsmry_loader reads back values from a UNSMRY") {
    GIVEN("A summary case with FOPT, BPR:567 and WWCT:OP-1 over 4 steps") {
        WriteSpec spec;
        spec.num_report_steps = 4;
        spec.num_ministep = 1;
        const auto case_path = (dirname / "CASE").string();
        const bool fmt_output = false;
        const bool unified = true;
        const bool lazy = GENERATE(true, false);
        write_test_summary(case_path, spec, fmt_output, unified);
        auto rd_sum = read_summary(case_path, ":", lazy);
        REQUIRE(rd_sum);

        THEN("The SMSPEC and UNSMRY files are written") {
            REQUIRE(fs::exists(case_path + ".SMSPEC"));
            REQUIRE(fs::exists(case_path + ".UNSMRY"));
        }

        WHEN("A rd::unsmry_loader is constructed over the UNSMRY file") {
            auto loader = std::make_unique<rd::unsmry_loader>(
                rd_sum_get_smspec(rd_sum.get()), case_path + ".UNSMRY", 0);

            THEN("get_vector returns the per-keyword series") {
                const std::vector<double> fopt = loader->get_vector(1);
                const std::vector<double> bpr = loader->get_vector(2);
                const std::vector<double> wwct = loader->get_vector(3);

                REQUIRE(fopt.size() == size_t(spec.num_report_steps));
                REQUIRE_THAT(fopt[3], WithinAbs(259200.0, 1e-6));
                REQUIRE_THAT(bpr[2], WithinAbs(1728000.0, 1e-6));
                REQUIRE_THAT(wwct[1], WithinAbs(8640000.0, 1e-6));
            }
        }
    }
}

SCENARIO_METHOD(Tmpdir, "rd_sum_alloc_resample over a time vector") {
    GIVEN("A summary case sampled at sim_days 1, 3, 5, 7") {
        WriteSpec spec;
        spec.num_report_steps = 4;
        spec.num_ministep = 1;
        spec.ministep_length = 2 * 86400.0;
        spec.start_seconds = 86400.0;
        const auto case_path = (dirname / "CASE").string();
        write_test_summary(case_path, spec, /*fmt_output=*/false,
                           /*unified=*/true);
        auto rd_sum = read_summary(case_path);
        REQUIRE(rd_sum);

        // write_test_summary writes:
        //   FOPT[i]    = sim_seconds = i * 259200       (variable)
        //   BPR:567[i] = 10  * sim_seconds              (variable)
        //   WWCT:OP-1  = 100 * sim_seconds              (rate)

        WHEN("Resampling to days 2, 4, 6 (interpolation only)") {
            std::vector<int> days{2, 4, 6};
            auto times = make_time_t_vector(0, 0);
            for (auto &d : days)
                time_t_vector_append(times.get(),
                                     util_make_date_utc(d, 1, 2010));

            auto resampled = rd_sum_alloc_resample(rd_sum.get(), "kk",
                                                   times.get(), false, false);
            REQUIRE(resampled);

            THEN("Report times line up with the requested vector") {
                for (size_t i = 0; i < days.size(); i++)
                    REQUIRE(rd_sum_get_report_time(resampled.get(), i) ==
                            util_make_date_utc(days[i], 1, 2010));
            }

            const rd_smspec_type *smspec = rd_sum_get_smspec(resampled.get());
            const rd::smspec_node &fopt =
                rd_smspec_iget_node_w_params_index(smspec, 1);
            const rd::smspec_node &bpr =
                rd_smspec_iget_node_w_params_index(smspec, 2);
            const rd::smspec_node &wwct =
                rd_smspec_iget_node_w_params_index(smspec, 3);

            THEN("Smspec metadata is preserved") {
                REQUIRE(std::string(smspec_node_get_keyword(&bpr)) == "BPR");
                REQUIRE(std::string(smspec_node_get_unit(&bpr)) == "BARS");
            }

            THEN("Interpolated values match a linear model") {
                // FOPT at sim_day 5: between (day 3, 259200) and (day 6, 518400)
                REQUIRE_THAT(
                    rd_sum_get_from_sim_time(
                        resampled.get(), util_make_date_utc(6, 1, 2010), &fopt),
                    WithinAbs(432000.0, 1e-3));
                // BPR at sim_day 1: between (day 0, 0) and (day 3, 2592000)
                REQUIRE_THAT(
                    rd_sum_get_from_sim_time(
                        resampled.get(), util_make_date_utc(2, 1, 2010), &bpr),
                    WithinAbs(864000.0, 1e-3));
                // WWCT exactly at sim_day 3
                REQUIRE_THAT(
                    rd_sum_get_from_sim_time(
                        resampled.get(), util_make_date_utc(4, 1, 2010), &wwct),
                    WithinAbs(25920000.0, 1e-3));
            }
        }

        WHEN("Resampling to days outside range (upper and lower "
             "extrapolation)") {
            auto times = make_time_t_vector(0, 0);
            time_t_vector_append(times.get(), util_make_date_utc(1, 1, 2009));
            time_t_vector_append(times.get(), util_make_date_utc(4, 1, 2010));
            time_t_vector_append(times.get(), util_make_date_utc(12, 1, 2010));

            auto resampled =
                rd_sum_alloc_resample(rd_sum.get(), "kk", times.get(),
                                      /*lower_extrapolation=*/true,
                                      /*upper_extrapolation=*/true);
            REQUIRE(resampled);

            const rd_smspec_type *smspec = rd_sum_get_smspec(resampled.get());
            const rd::smspec_node &fopt =
                rd_smspec_iget_node_w_params_index(smspec, 1);
            const rd::smspec_node &wwct =
                rd_smspec_iget_node_w_params_index(smspec, 3);

            THEN("Rate variables extrapolate to zero outside the range") {
                REQUIRE_THAT(
                    rd_sum_get_from_sim_time(
                        resampled.get(), util_make_date_utc(1, 1, 2009), &wwct),
                    WithinAbs(0.0, 1e-6));
                REQUIRE_THAT(
                    rd_sum_get_from_sim_time(
                        resampled.get(), util_make_date_utc(4, 1, 2010), &wwct),
                    WithinAbs(25920000.0, 1e-3));
                REQUIRE_THAT(rd_sum_get_from_sim_time(
                                 resampled.get(),
                                 util_make_date_utc(12, 1, 2010), &wwct),
                             WithinAbs(0.0, 1e-6));
            }

            THEN("Non-rate variables hold the boundary values") {
                REQUIRE_THAT(
                    rd_sum_get_from_sim_time(
                        resampled.get(), util_make_date_utc(1, 1, 2009), &fopt),
                    WithinAbs(86400.0, 1e-6));
                REQUIRE_THAT(
                    rd_sum_get_from_sim_time(
                        resampled.get(), util_make_date_utc(4, 1, 2010), &fopt),
                    WithinAbs(259200.0, 1e-3));
                REQUIRE_THAT(rd_sum_get_from_sim_time(
                                 resampled.get(),
                                 util_make_date_utc(12, 1, 2010), &fopt),
                             WithinAbs(604800.0, 1e-3));
            }
        }

        WHEN("Resampling against an unsorted time vector") {
            auto times = make_time_t_vector(0, 0);
            time_t_vector_append(times.get(), util_make_date_utc(1, 1, 2010));
            time_t_vector_append(times.get(), util_make_date_utc(3, 1, 2010));
            time_t_vector_append(times.get(), util_make_date_utc(2, 1, 2010));

            THEN("rd_sum_alloc_resample returns null") {
                REQUIRE_FALSE(rd_sum_alloc_resample(rd_sum.get(), "kk",
                                                    times.get(), false, false));
            }
        }
    }

    GIVEN("A summary case sampled at sim_days 0, 1, 2, 3") {
        WriteSpec spec;
        spec.num_report_steps = 4;
        spec.num_ministep = 1;
        spec.ministep_length = 86400.0;
        const auto case_path = (dirname / "CASE").string();
        write_test_summary(case_path, spec, /*fmt_output=*/false,
                           /*unified=*/true);
        auto rd_sum = read_summary(case_path);
        REQUIRE(rd_sum);

        WHEN("Resampling to days 2, 4, 6, 8 (upper_extrapolation only)") {
            std::vector<int> days{2, 4, 6, 8};
            auto times = make_time_t_vector(0, 0);
            for (auto &d : days)
                time_t_vector_append(times.get(),
                                     util_make_date_utc(d, 1, 2010));

            auto resampled = rd_sum_alloc_resample(rd_sum.get(), "kk",
                                                   times.get(), false, true);
            REQUIRE(resampled);

            THEN("Report times line up with the requested vector") {
                for (size_t i = 0; i < days.size(); i++) {
                    time_t date = util_make_date_utc(days[i], 1, 2010);
                    REQUIRE(rd_sum_get_report_time(resampled.get(), i) == date);
                    REQUIRE_THAT(rd_sum_time2days(resampled.get(), date),
                                 WithinAbs(days[i] - 2, 1e-6));
                }
            }

            const rd_smspec_type *smspec = rd_sum_get_smspec(resampled.get());
            const rd::smspec_node &fopt =
                rd_smspec_iget_node_w_params_index(smspec, 1);
            const rd::smspec_node &bpr =
                rd_smspec_iget_node_w_params_index(smspec, 2);
            const rd::smspec_node &wwct =
                rd_smspec_iget_node_w_params_index(smspec, 3);

            THEN("Interpolated values match a linear model") {
                // FOPT extrapolated to sim_day 5
                REQUIRE_THAT(
                    rd_sum_get_from_sim_time(
                        resampled.get(), util_make_date_utc(6, 1, 2010), &fopt),
                    WithinAbs(259200.0, 1e-3));
                // BPR at exactly sim_day 1
                REQUIRE_THAT(
                    rd_sum_get_from_sim_time(
                        resampled.get(), util_make_date_utc(2, 1, 2010), &bpr),
                    WithinAbs(864000.0, 1e-3));
                // WWCT exactly at sim_day 3
                REQUIRE_THAT(
                    rd_sum_get_from_sim_time(
                        resampled.get(), util_make_date_utc(4, 1, 2010), &wwct),
                    WithinAbs(25920000.0, 1e-3));
            }
        }
    }
}

TEST_CASE_METHOD(Tmpdir, "fread_alloc_case guesses base when given directory") {
    GIVEN("A summary case written into a subdirectory with a .DATA file") {
        const fs::path subdir = dirname / "sub";
        fs::create_directory(subdir);

        const std::string base = "CASE";
        const auto case_path = (subdir / base).string();
        WriteSpec spec;
        spec.num_report_steps = 2;
        spec.num_ministep = 2;

        const std::string unique_well = "UNIQ-WELL-XYZ-42";
        const std::string unique_key = "WOPR:" + unique_well;
        const float unique_scale = 17.5f;
        {
            auto rd_sum =
                make_summary_writer(case_path,
                                    /*fmt_output=*/false,
                                    /*unified=*/true, ":", spec.start_time,
                                    true, spec.nx, spec.ny, spec.nz);
            const rd::smspec_node *wopr = rd_sum_add_var(
                rd_sum.get(), "WOPR", unique_well.c_str(), 0, "SM3/DAY", 0.0f);
            double sim_seconds = spec.start_seconds;
            for (int report_step = 0; report_step < spec.num_report_steps;
                 ++report_step) {
                for (int step = 0; step < spec.num_ministep; ++step) {
                    rd_sum_tstep_type *tstep = rd_sum_add_tstep(
                        rd_sum.get(), report_step + 1, sim_seconds);
                    rd_sum_tstep_set_from_node(tstep, *wopr,
                                               unique_scale * sim_seconds);
                    sim_seconds += spec.ministep_length;
                }
            }
            rd_sum_fwrite(rd_sum.get());
        }

        // Empty DATA file so base_guess can identify the case.
        std::ofstream((subdir / (base + ".DATA")).string()).close();

        WHEN("loading via read_summary with the directory path") {
            const std::string dir_path = subdir.string() + "/";
            auto guessed = read_summary(dir_path);
            REQUIRE(guessed.get() != nullptr);

            AND_WHEN(
                "loading the same files explicitly via rd_sum_fread_alloc") {
                const std::string header_file =
                    (subdir / (base + ".SMSPEC")).string();
                auto data_files = make_stringlist();
                stringlist_append_copy(
                    data_files.get(),
                    (subdir / (base + ".UNSMRY")).string().c_str());

                auto explicit_sum = rd_sum_ptr(
                    rd_sum_fread_alloc(header_file.c_str(), data_files.get(),
                                       ":", /*include_restart=*/true,
                                       /*lazy_load=*/true,
                                       /*file_options=*/0),
                    &rd_sum_free);
                REQUIRE(explicit_sum.get() != nullptr);

                THEN("both methods loads the same case") {
                    REQUIRE(rd_sum_has_general_var(guessed.get(),
                                                   unique_key.c_str()));
                    REQUIRE(rd_sum_has_general_var(explicit_sum.get(),
                                                   unique_key.c_str()));

                    REQUIRE(rd_sum_get_start_time(guessed.get()) ==
                            rd_sum_get_start_time(explicit_sum.get()));
                    REQUIRE(rd_sum_get_end_time(guessed.get()) ==
                            rd_sum_get_end_time(explicit_sum.get()));

                    const int n = rd_sum_get_data_length(guessed.get());
                    REQUIRE(n == rd_sum_get_data_length(explicit_sum.get()));
                    REQUIRE(n == spec.num_report_steps * spec.num_ministep);

                    REQUIRE(std::string(rd_sum_get_unit(guessed.get(),
                                                        unique_key.c_str())) ==
                            "SM3/DAY");
                    REQUIRE(std::string(rd_sum_get_unit(explicit_sum.get(),
                                                        unique_key.c_str())) ==
                            "SM3/DAY");

                    for (int i = 0; i < n; ++i) {
                        const double expected =
                            unique_scale * i * spec.ministep_length;
                        REQUIRE_THAT(rd_sum_get_general_var(guessed.get(), i,
                                                            unique_key.c_str()),
                                     WithinAbs(expected, 1e-3));
                        REQUIRE_THAT(rd_sum_get_general_var(explicit_sum.get(),
                                                            i,
                                                            unique_key.c_str()),
                                     WithinAbs(expected, 1e-3));
                    }

                    auto wells = stringlist_ptr(
                        rd_sum_alloc_well_list(guessed.get(), nullptr),
                        &stringlist_free);
                    REQUIRE(stringlist_get_size(wells.get()) == 1);
                    REQUIRE(std::string(stringlist_iget(wells.get(), 0)) ==
                            unique_well);
                }

                THEN("rd_sum_get_base returns nullptr for the guessed case") {
                    REQUIRE(rd_sum_get_base(guessed.get()) == nullptr);
                    REQUIRE(std::string(rd_sum_get_base(explicit_sum.get())) ==
                            base);
                }
            }
        }
    }
}

namespace {
std::vector<std::string> split(const std::string &line, char sep) {
    std::vector<std::string> out;
    std::string cur;
    for (char c : line) {
        if (c == sep) {
            out.push_back(cur);
            cur.clear();
        } else {
            cur.push_back(c);
        }
    }
    out.push_back(cur);
    return out;
}
} // namespace

TEST_CASE_METHOD(Tmpdir, "rd_sum_export_csv writes the requested keys") {
    WriteSpec spec;
    spec.num_report_steps = 2;
    spec.num_ministep = 3;
    auto case_path = (dirname / "CASE").string();
    write_test_summary(case_path, spec, /*fmt_output=*/false,
                       /*unified=*/true);
    auto rd_sum = read_summary(case_path);
    REQUIRE(rd_sum.get() != nullptr);
    const int n = rd_sum_get_data_length(rd_sum.get());

    SECTION("subset of keys with comma separator and ISO date") {
        auto vars = make_stringlist();
        stringlist_append_copy(vars.get(), "FOPT");
        stringlist_append_copy(vars.get(), "BPR:567");

        const auto csv_path = (dirname / "out.csv").string();
        rd_sum_export_csv(rd_sum.get(), csv_path.c_str(), vars.get(),
                          "%Y-%m-%d", ",");

        REQUIRE(fs::exists(csv_path));
        std::ifstream in(csv_path, std::ios::binary);
        std::string content((std::istreambuf_iterator<char>(in)),
                            std::istreambuf_iterator<char>());
        REQUIRE(content.find("\r\n") != std::string::npos);

        std::vector<std::string> lines;
        {
            std::string line;
            std::stringstream ss(content);
            while (std::getline(ss, line)) {
                if (!line.empty() && line.back() == '\r')
                    line.pop_back();
                if (!line.empty())
                    lines.push_back(line);
            }
        }
        REQUIRE(lines.size() == static_cast<size_t>(n + 1));

        const auto header = split(lines[0], ',');
        REQUIRE(header.size() == 4);
        REQUIRE(header[0] == "DAYS");
        REQUIRE(header[1] == "DATE");
        REQUIRE(header[2] == "FOPT");
        REQUIRE(header[3] == "BPR:567");

        for (int i = 0; i < n; ++i) {
            const auto cols = split(lines[i + 1], ',');
            REQUIRE(cols.size() == 4);
            REQUIRE_THAT(
                std::stod(cols[0]),
                WithinAbs(rd_sum_iget_sim_days(rd_sum.get(), i), 1e-2));
            REQUIRE(cols[1] == "2010-01-0" + std::to_string(i + 1));
            REQUIRE_THAT(
                std::stod(cols[2]),
                WithinAbs(rd_sum_get_general_var(rd_sum.get(), i, "FOPT"),
                          1e-3));
            REQUIRE_THAT(
                std::stod(cols[3]),
                WithinAbs(rd_sum_get_general_var(rd_sum.get(), i, "BPR:567"),
                          1e-3));
        }
    }

    SECTION("missing keys are skipped and parent dirs are created") {
        auto vars = make_stringlist();
        stringlist_append_copy(vars.get(), "FOPT");
        stringlist_append_copy(vars.get(), "NOPE:DOES_NOT_EXIST");
        stringlist_append_copy(vars.get(), "WWCT:OP-1");

        const auto csv_path = (dirname / "csv-out" / "out.csv").string();
        rd_sum_export_csv(rd_sum.get(), csv_path.c_str(), vars.get(),
                          "%d/%m/%Y", ";");
        REQUIRE(fs::exists(csv_path));

        std::ifstream in(csv_path, std::ios::binary);
        std::string first_line;
        std::getline(in, first_line);
        if (!first_line.empty() && first_line.back() == '\r')
            first_line.pop_back();

        const auto header = split(first_line, ';');
        REQUIRE(header.size() == 4);
        REQUIRE(header[0] == "DAYS");
        REQUIRE(header[1] == "DATE");
        REQUIRE(header[2] == "FOPT");
        REQUIRE(header[3] == "WWCT:OP-1");
    }
}
