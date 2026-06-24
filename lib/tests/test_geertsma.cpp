#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <filesystem>
#include <memory>
#include <optional>
#include <stdexcept>
#include <vector>

#include <resdata/FortIO.hpp>
#include <resdata/rd_file.hpp>
#include <resdata/rd_file_view.hpp>
#include <resdata/rd_grav.hpp>
#include <resdata/rd_grid.hpp>
#include <resdata/rd_kw_magic.hpp>
#include <resdata/rd_subsidence.hpp>
#include <resdata/rd_util.hpp>

#include "grid_fixtures.hpp"
#include "tmpdir.hpp"

using namespace Catch::Matchers;

namespace {

/** Writes an INIT file containing PORO and PORV keywords. */
void write_subsidence_init(const fs::path &filename, int size) {
    auto fortio = make_fortio_writer(filename);

    std::vector<float> poro(size, 0.0f);
    write_float_kw(fortio, PORO_KW, poro.data(), size);

    /* PORV is not used by the Geertsma kernel (which uses the grid cell
       volume), only by the biot-savart rd_subsidence_eval. */
    std::vector<float> porv(size, 0.0f);
    write_float_kw(fortio, PORV_KW, porv.data(), size);
}

void write_restart_block(ERT::FortIO &fortio, int seqnum, int year,
                         const std::vector<float> &pressure,
                         const std::optional<std::vector<float>> &rporv) {
    write_int_kw(fortio, SEQNUM_KW, std::vector<int>{seqnum});

    std::vector<int> intehead(67, 0);
    intehead[INTEHEAD_DAY_INDEX] = 1;
    intehead[INTEHEAD_MONTH_INDEX] = 1;
    intehead[INTEHEAD_YEAR_INDEX] = year;
    write_int_kw(fortio, INTEHEAD_KW, intehead);

    write_float_kw(fortio, PRESSURE_KW, pressure.data(),
                   static_cast<int>(pressure.size()));

    if (rporv.has_value())
        write_float_kw(fortio, RPORV_KW, rporv->data(),
                       static_cast<int>(rporv->size()));
}

/** Writes a UNRST file with one or two restart blocks. */
void write_subsidence_restart(
    const fs::path &filename, const std::vector<float> &p1,
    const std::optional<std::vector<float>> &p2 = std::nullopt,
    const std::optional<std::vector<float>> &rporv1 = std::nullopt,
    const std::optional<std::vector<float>> &rporv2 = std::nullopt) {
    auto fortio = make_fortio_writer(filename);

    write_restart_block(fortio, 10, 2000, p1, rporv1);
    if (p2.has_value())
        write_restart_block(fortio, 20, 2010, *p2, rporv2);
}

} // namespace

TEST_CASE_METHOD(Tmpdir, "Geertsma kernel single cell") {
    auto grid = make_rectangular_grid(1, 1, 1, 50.0, 50.0, 50.0, nullptr);
    const int size = rd_grid_get_active_size(grid.get());

    auto init_path = dirname / "TEST.INIT";
    auto unrst_path = dirname / "TEST.UNRST";

    write_subsidence_init(init_path, size);
    write_subsidence_restart(unrst_path, std::vector<float>{1.0f});

    rd_file_ptr init = open_rd_file(init_path, 0);
    rd_file_ptr restart = open_rd_file(unrst_path, 0);

    std::unique_ptr<rd_subsidence_type, decltype(&rd_subsidence_free)>
        subsidence(rd_subsidence_alloc(grid.get(), init.get()),
                   rd_subsidence_free);

    rd_file_view_type *view1 =
        rd_file_get_restart_view(restart.get(), 0, -1, -1, -1);
    REQUIRE(view1 != nullptr);

    rd_subsidence_add_survey_PRESSURE(subsidence.get(), "S1", view1);
    rd_subsidence_add_survey_PRESSURE(subsidence.get(), "S2", view1);

    const double youngs_modulus = 5e8;
    const double poisson_ratio = 0.3;
    const double seabed = 0.0;
    const double above = 100.0;
    const double topres = 2000.0;

    SECTION("receiver at surface") {
        double dz = rd_subsidence_eval_geertsma(
            subsidence.get(), "S1", nullptr, nullptr, 1000, 1000, 0,
            youngs_modulus, poisson_ratio, seabed);
        REQUIRE_THAT(dz, WithinRel(3.944214576168326e-09, 1e-9));
    }

    SECTION("receiver below surface") {
        double depth = topres - seabed - above;
        double dz = rd_subsidence_eval_geertsma(
            subsidence.get(), "S1", nullptr, nullptr, 1000, 1000, depth,
            youngs_modulus, poisson_ratio, seabed);
        REQUIRE_THAT(dz, WithinRel(5.8160298201497136e-08, 1e-9));

        double dz_diff =
            rd_subsidence_eval(subsidence.get(), "S1", "S2", nullptr, 1000,
                               1000, depth, youngs_modulus, poisson_ratio);
        REQUIRE_THAT(dz_diff, WithinAbs(0.0, 1e-12));
    }
}

TEST_CASE_METHOD(Tmpdir, "Geertsma kernel two source points two vintages") {
    auto grid = make_rectangular_grid(2, 1, 1, 100.0, 100.0, 100.0, nullptr);

    auto init_path = dirname / "TEST.INIT";
    auto unrst_path = dirname / "TEST.UNRST";

    write_subsidence_init(init_path, rd_grid_get_active_size(grid.get()));
    write_subsidence_restart(unrst_path, std::vector<float>{1.0f, 10.0f},
                             std::vector<float>{10.0f, 20.0f});

    rd_file_ptr init = open_rd_file(init_path, 0);
    rd_file_ptr restart = open_rd_file(unrst_path, 0);

    std::unique_ptr<rd_subsidence_type, decltype(&rd_subsidence_free)>
        subsidence(rd_subsidence_alloc(grid.get(), init.get()),
                   rd_subsidence_free);

    rd_file_view_type *view1 =
        rd_file_get_restart_view(restart.get(), 0, -1, -1, -1);
    rd_file_view_type *view2 =
        rd_file_get_restart_view(restart.get(), 1, -1, -1, -1);
    REQUIRE(view1 != nullptr);
    REQUIRE(view2 != nullptr);

    rd_subsidence_add_survey_PRESSURE(subsidence.get(), "S1", view1);
    rd_subsidence_add_survey_PRESSURE(subsidence.get(), "S2", view2);

    const double youngs_modulus = 5e8;
    const double poisson_ratio = 0.3;
    const double seabed = 0.0;

    double dz1 = rd_subsidence_eval_geertsma(
        subsidence.get(), "S1", nullptr, nullptr, 1000, 1000, 0, youngs_modulus,
        poisson_ratio, seabed);
    REQUIRE_THAT(dz1, WithinRel(8.65322541521704e-07, 1e-9));

    double dz2 = rd_subsidence_eval_geertsma(
        subsidence.get(), "S2", nullptr, nullptr, 1000, 1000, 0, youngs_modulus,
        poisson_ratio, seabed);
    REQUIRE_THAT(dz2, WithinRel(2.275556615015282e-06, 1e-9));

    double dz = rd_subsidence_eval_geertsma(
        subsidence.get(), "S1", "S2", nullptr, 1000, 1000, 0, youngs_modulus,
        poisson_ratio, seabed);
    REQUIRE_THAT(dz, WithinRel(dz1 - dz2, 1e-9));
}

TEST_CASE_METHOD(Tmpdir, "Geertsma kernel with seabed") {
    auto grid = make_rectangular_grid(1, 1, 1, 50.0, 50.0, 50.0, nullptr);

    auto init_path = dirname / "TEST.INIT";
    auto unrst_path = dirname / "TEST.UNRST";

    write_subsidence_init(init_path, rd_grid_get_active_size(grid.get()));
    write_subsidence_restart(unrst_path, std::vector<float>{1.0f});

    rd_file_ptr init = open_rd_file(init_path, 0);
    rd_file_ptr restart = open_rd_file(unrst_path, 0);

    std::unique_ptr<rd_subsidence_type, decltype(&rd_subsidence_free)>
        subsidence(rd_subsidence_alloc(grid.get(), init.get()),
                   rd_subsidence_free);

    rd_file_view_type *view1 =
        rd_file_get_restart_view(restart.get(), 0, -1, -1, -1);
    rd_subsidence_add_survey_PRESSURE(subsidence.get(), "S1", view1);

    const double youngs_modulus = 5e8;
    const double poisson_ratio = 0.3;
    const double seabed = 300.0;
    const double above = 100.0;
    const double topres = 2000.0;
    double depth = topres - seabed - above;

    double dz = rd_subsidence_eval_geertsma(
        subsidence.get(), "S1", nullptr, nullptr, 1000, 1000, depth,
        youngs_modulus, poisson_ratio, seabed);
    REQUIRE_THAT(dz, WithinRel(5.819790154474284e-08, 1e-9));
}

TEST_CASE_METHOD(Tmpdir,
                 "Geertsma rporv kernel two source points two vintages") {
    auto grid = make_rectangular_grid(2, 1, 1, 100.0, 100.0, 100.0, nullptr);

    auto init_path = dirname / "TEST.INIT";
    auto unrst_path = dirname / "TEST.UNRST";

    write_subsidence_init(init_path, rd_grid_get_active_size(grid.get()));
    write_subsidence_restart(unrst_path, std::vector<float>{1.0f, 10.0f},
                             std::vector<float>{10.0f, 20.0f},
                             std::vector<float>{1e5f, 1e5f},
                             std::vector<float>{9e4f, 9e4f});

    rd_file_ptr init = open_rd_file(init_path, 0);
    rd_file_ptr restart = open_rd_file(unrst_path, 0);

    std::unique_ptr<rd_subsidence_type, decltype(&rd_subsidence_free)>
        subsidence(rd_subsidence_alloc(grid.get(), init.get()),
                   rd_subsidence_free);

    rd_file_view_type *view1 =
        rd_file_get_restart_view(restart.get(), 0, -1, -1, -1);
    rd_file_view_type *view2 =
        rd_file_get_restart_view(restart.get(), 1, -1, -1, -1);

    rd_subsidence_add_survey_PRESSURE(subsidence.get(), "S1", view1);
    rd_subsidence_add_survey_PRESSURE(subsidence.get(), "S2", view2);

    const double youngs_modulus = 5e8;
    const double poisson_ratio = 0.3;
    const double seabed = 0.0;

    double dz1 = rd_subsidence_eval_geertsma_rporv(
        subsidence.get(), "S1", nullptr, nullptr, 1000, 1000, 0, youngs_modulus,
        poisson_ratio, seabed);
    double dz2 = rd_subsidence_eval_geertsma_rporv(
        subsidence.get(), "S2", nullptr, nullptr, 1000, 1000, 0, youngs_modulus,
        poisson_ratio, seabed);
    double dz = rd_subsidence_eval_geertsma_rporv(
        subsidence.get(), "S1", "S2", nullptr, 1000, 1000, 0, youngs_modulus,
        poisson_ratio, seabed);

    REQUIRE_THAT(dz, WithinRel(dz1 - dz2, 1e-9));
    REQUIRE(dz > 0);
}

TEST_CASE_METHOD(Tmpdir, "Subsidence survey validation") {
    auto grid = make_rectangular_grid(2, 1, 1, 100.0, 100.0, 100.0, nullptr);

    auto init_path = dirname / "TEST.INIT";
    auto unrst_path = dirname / "TEST.UNRST";

    write_subsidence_init(init_path, rd_grid_get_active_size(grid.get()));
    write_subsidence_restart(unrst_path, std::vector<float>{1.0f, 10.0f},
                             std::vector<float>{10.0f, 20.0f},
                             std::vector<float>{1e5f, 1e5f},
                             std::vector<float>{9e4f, 9e4f});

    rd_file_ptr init = open_rd_file(init_path, 0);
    rd_file_ptr restart = open_rd_file(unrst_path, 0);

    std::unique_ptr<rd_subsidence_type, decltype(&rd_subsidence_free)>
        subsidence(rd_subsidence_alloc(grid.get(), init.get()),
                   rd_subsidence_free);

    const double youngs_modulus = 5e8;
    const double compressibility = 1000;
    const double poisson_ratio = 0.3;
    const double seabed = 0.0;

    SECTION("unknown base survey throws") {
        REQUIRE(!rd_subsidence_has_survey(subsidence.get(), "dummy_survey"));
        REQUIRE_THROWS_AS(rd_subsidence_eval_geertsma_rporv(
                              subsidence.get(), "dummy_survey", nullptr,
                              nullptr, 1000, 1000, 0, youngs_modulus,
                              poisson_ratio, seabed),
                          std::out_of_range);
        REQUIRE_THROWS_AS(rd_subsidence_eval(subsidence.get(), "dummy_survey",
                                             nullptr, nullptr, 1000, 1000, 0,
                                             compressibility, poisson_ratio),
                          std::out_of_range);
    }

    SECTION("unknown monitor survey throws") {
        rd_file_view_type *view1 =
            rd_file_get_restart_view(restart.get(), 0, -1, -1, -1);
        rd_subsidence_add_survey_PRESSURE(subsidence.get(), "S1", view1);
        REQUIRE(rd_subsidence_has_survey(subsidence.get(), "S1"));

        REQUIRE_THROWS_AS(rd_subsidence_eval_geertsma_rporv(
                              subsidence.get(), "S1", "dummy_survey", nullptr,
                              1000, 1000, 0, youngs_modulus, poisson_ratio,
                              seabed),
                          std::out_of_range);
        REQUIRE_THROWS_AS(
            rd_subsidence_eval(subsidence.get(), "S1", "dummy_survey", nullptr,
                               1000, 1000, 0, compressibility, poisson_ratio),
            std::out_of_range);
    }
}
