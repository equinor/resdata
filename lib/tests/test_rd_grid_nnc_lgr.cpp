#include <catch2/catch.hpp>
#include <algorithm>
#include <memory>
#include <resdata/rd_grid.hpp>
#include <resdata/rd_kw.hpp>
#include <resdata/rd_kw_magic.hpp>
#include <resdata/nnc_info.hpp>
#include <resdata/nnc_vector.hpp>
#include <vector>

#include "grid_fixtures.hpp"
#include "tmpdir.hpp"

using namespace Catch;
using namespace Matchers;

TEST_CASE_METHOD(Tmpdir, "Load EGRID with a single LGR", "[unittest]") {
    GIVEN("An EGRID file containing a main grid and one LGR") {
        auto filename = dirname / "LGR.EGRID";
        auto grid = load_egrid_with_single_lgr(filename, 3, 3, 3, 2, 2, 2, 1, 1,
                                               1, "LGR1");

        THEN("The grid has the LGR") {
            REQUIRE(grid != nullptr);
            REQUIRE(rd_grid_get_num_lgr(grid.get()) == 1);
            REQUIRE(rd_grid_has_lgr(grid.get(), "LGR1"));

            rd_grid_type *lgr = rd_grid_get_lgr(grid.get(), "LGR1");
            REQUIRE(lgr != nullptr);
            REQUIRE(rd_grid_get_nx(lgr) == 2);
            REQUIRE(rd_grid_get_ny(lgr) == 2);
            REQUIRE(rd_grid_get_nz(lgr) == 2);
            REQUIRE(rd_grid_get_lgr_nr(lgr) == 1);

            AND_THEN("rd_grid_iget_lgr returns the same LGR grid as the "
                     "name-based lookup") {
                REQUIRE(rd_grid_iget_lgr(grid.get(), 0) == lgr);
            }

            AND_THEN("The LGR tree is reported as consistent") {
                REQUIRE(rd_grid_test_lgr_consistency(grid.get()));
            }

            AND_THEN("The main grid is single-porosity, so fracture queries "
                     "returns -1") {
                REQUIRE(rd_grid_get_active_fracture_index1(grid.get(), 0) ==
                        -1);
                REQUIRE(rd_grid_get_global_index1F(grid.get(), 0) == -1);
            }

            AND_THEN("rd_grid_init_actnum_data writes one entry per main-grid "
                     "cell matching the per-cell active status") {
                const int size = rd_grid_get_global_size(grid.get());
                std::vector<int> actnum(size);
                rd_grid_init_actnum_data(grid.get(), actnum.data());
                for (int i = 0; i < size; i++)
                    REQUIRE(actnum[i] ==
                            (rd_grid_cell_active1(grid.get(), i) ? 1 : 0));
            }

            AND_THEN("Read/write preserves the lgr") {
                auto grid_filename = dirname / "LGR.GRID";
                rd_grid_fwrite_GRID2(grid.get(), grid_filename.c_str(),
                                     RD_METRIC_UNITS);
                REQUIRE(fs::exists(grid_filename));

                auto reloaded = read_grid(grid_filename);
                REQUIRE(reloaded != nullptr);
                REQUIRE(rd_grid_get_num_lgr(reloaded.get()) == 1);
                REQUIRE(rd_grid_has_lgr(reloaded.get(), "LGR1"));
            }

            AND_THEN("Copying the grid preserves the LGR") {
                auto copy =
                    rd_grid_ptr(rd_grid_alloc_copy(grid.get()), &rd_grid_free);
                REQUIRE(copy != nullptr);
                REQUIRE(rd_grid_get_num_lgr(copy.get()) == 1);
                REQUIRE(rd_grid_has_lgr(copy.get(), "LGR1"));
            }
        }
    }
}

TEST_CASE_METHOD(Tmpdir,
                 "Load EGRID with NNCs between the main grid and an LGR",
                 "[unittest]") {
    // The NNCHEAD section for the LGR references global-grid cells via NNCG
    // and LGR cells via NNCL.
    const std::vector<int> nncg = {1};
    const std::vector<int> nncl = {1};

    GIVEN("A grid with a single LGR and an NNCG/NNCL pair") {
        auto filename = dirname / "LGR_NNC.EGRID";
        auto grid = load_egrid_with_single_lgr(filename, 3, 3, 3, 2, 2, 2, 1, 1,
                                               1, "LGR1", nullptr, nncg, nncl);

        THEN("The grid has the lgr") {
            REQUIRE(grid != nullptr);
            REQUIRE(rd_grid_has_lgr(grid.get(), "LGR1"));
        }
    }
}

TEST_CASE_METHOD(Tmpdir, "Load EGRID with nested LGRs", "[unittest]") {
    GIVEN("An EGRID file with an outer LGR and an inner LGR nested in it") {
        auto filename = dirname / "NESTED.EGRID";
        auto grid =
            load_egrid_with_nested_lgr(filename, 3, 3, 3, 1, 1, 1, 2, 2, 2,
                                       "OUTER", 0, 0, 0, 2, 2, 2, "INNER");

        THEN("The grid can be loaded and has both LGRs") {
            REQUIRE(grid != nullptr);
            REQUIRE(rd_grid_get_num_lgr(grid.get()) == 2);
            REQUIRE(rd_grid_has_lgr(grid.get(), "OUTER"));
            REQUIRE(rd_grid_has_lgr(grid.get(), "INNER"));

            AND_THEN("Copying the grid preserves both LGRs") {
                auto copy =
                    rd_grid_ptr(rd_grid_alloc_copy(grid.get()), &rd_grid_free);
                REQUIRE(copy != nullptr);
                REQUIRE(rd_grid_get_num_lgr(copy.get()) == 2);
                REQUIRE(rd_grid_has_lgr(copy.get(), "OUTER"));
                REQUIRE(rd_grid_has_lgr(copy.get(), "INNER"));
            }
        }
    }
}

TEST_CASE_METHOD(Tmpdir, "Load GRID file with two-element LGR_KW",
                 "[unittest]") {
    GIVEN("A GRID file whose LGR_KW has an empty parent name") {
        auto filename = dirname / "LGR_EMPTY.GRID";
        auto grid = load_grid_file_with_lgr_parent(filename, "LGR1", "");

        THEN("The file loads and has the LGR") {
            REQUIRE(grid != nullptr);
            REQUIRE(rd_grid_get_num_lgr(grid.get()) == 1);
            REQUIRE(rd_grid_has_lgr(grid.get(), "LGR1"));
        }
    }

    GIVEN("A GRID file whose LGR_KW declares parent = \"GLOBAL\"") {
        auto filename = dirname / "LGR_GLOBAL.GRID";
        auto grid =
            load_grid_file_with_lgr_parent(filename, "LGR1", GLOBAL_STRING);

        THEN("The file loads and the main grid has the LGR") {
            REQUIRE(grid != nullptr);
            REQUIRE(rd_grid_get_num_lgr(grid.get()) == 1);
            REQUIRE(rd_grid_has_lgr(grid.get(), "LGR1"));
        }
    }

    GIVEN("A GRID file with a nested LGR whose parent is another LGR") {
        auto filename = dirname / "LGR_NESTED.GRID";
        auto grid = load_grid_file_with_nested_lgr(filename, "OUTER", "INNER");

        THEN("The file loads and the nested LGR is hosted by the outer LGR") {
            REQUIRE(grid != nullptr);
            REQUIRE(rd_grid_get_num_lgr(grid.get()) == 2);
            REQUIRE(rd_grid_has_lgr(grid.get(), "OUTER"));
            REQUIRE(rd_grid_has_lgr(grid.get(), "INNER"));
        }
    }
}

TEST_CASE_METHOD(Tmpdir, "LGR name lookup functions", "[unittest]") {
    GIVEN("An EGRID file with two nested LGRs") {
        auto filename = dirname / "LGR_NAMES.EGRID";
        auto grid =
            load_egrid_with_nested_lgr(filename, 3, 3, 3, 1, 1, 1, 2, 2, 2,
                                       "OUTER", 0, 0, 0, 2, 2, 2, "INNER");
        REQUIRE(grid != nullptr);
        REQUIRE(rd_grid_get_num_lgr(grid.get()) == 2);

        THEN("rd_grid_iget_lgr_name returns the LGR names by index and NULL "
             "for out-of-range indices") {
            std::string first = rd_grid_iget_lgr_name(grid.get(), 0);
            std::string second = rd_grid_iget_lgr_name(grid.get(), 1);
            REQUIRE((first == "OUTER" || first == "INNER"));
            REQUIRE((second == "OUTER" || second == "INNER"));
            REQUIRE(first != second);
            REQUIRE(rd_grid_iget_lgr_name(grid.get(), 2) == nullptr);
        }

        THEN("rd_grid_get_lgr_name returns the main grid name for lgr_nr=0 "
             "and the LGR name for a valid lgr_nr") {
            std::string main_name = rd_grid_get_lgr_name(grid.get(), 0);
            REQUIRE(main_name == filename.string());

            int outer_nr = rd_grid_get_lgr_nr_from_name(grid.get(), "OUTER");
            int inner_nr = rd_grid_get_lgr_nr_from_name(grid.get(), "INNER");
            REQUIRE(outer_nr != 0);
            REQUIRE(inner_nr != 0);
            REQUIRE(outer_nr != inner_nr);

            std::string outer_name = rd_grid_get_lgr_name(grid.get(), outer_nr);
            std::string inner_name = rd_grid_get_lgr_name(grid.get(), inner_nr);
            REQUIRE(outer_name == "OUTER");
            REQUIRE(inner_name == "INNER");
        }

        THEN("rd_grid_get_lgr_nr_from_name returns 0 for the main grid's "
             "name") {
            REQUIRE(rd_grid_get_lgr_nr_from_name(grid.get(),
                                                 filename.c_str()) == 0);
        }

        THEN("rd_grid_has_lgr_nr returns true for each LGR's lgr_nr and "
             "false for an lgr_nr past the end of the map") {
            int outer_nr = rd_grid_get_lgr_nr_from_name(grid.get(), "OUTER");
            int inner_nr = rd_grid_get_lgr_nr_from_name(grid.get(), "INNER");
            REQUIRE(rd_grid_has_lgr_nr(grid.get(), outer_nr));
            REQUIRE(rd_grid_has_lgr_nr(grid.get(), inner_nr));
            int past_end = std::max(outer_nr, inner_nr) + 1;
            REQUIRE(!rd_grid_has_lgr_nr(grid.get(), past_end));
        }

        THEN("rd_grid_iget_lgr returns the same LGR pointer as the hash "
             "lookup rd_grid_get_lgr, at every valid index") {
            for (int i = 0; i < rd_grid_get_num_lgr(grid.get()); ++i) {
                const char *lgr_name = rd_grid_iget_lgr_name(grid.get(), i);
                REQUIRE(lgr_name != nullptr);
                rd_grid_type *by_index = rd_grid_iget_lgr(grid.get(), i);
                rd_grid_type *by_name = rd_grid_get_lgr(grid.get(), lgr_name);
                REQUIRE(by_index != nullptr);
                REQUIRE(by_index == by_name);
            }
        }

        THEN("rd_grid_get_lgr_from_lgr_nr resolves each LGR's lgr_nr back to "
             "the same pointer as rd_grid_get_lgr") {
            int outer_nr = rd_grid_get_lgr_nr_from_name(grid.get(), "OUTER");
            int inner_nr = rd_grid_get_lgr_nr_from_name(grid.get(), "INNER");
            REQUIRE(rd_grid_get_lgr_from_lgr_nr(grid.get(), outer_nr) ==
                    rd_grid_get_lgr(grid.get(), "OUTER"));
            REQUIRE(rd_grid_get_lgr_from_lgr_nr(grid.get(), inner_nr) ==
                    rd_grid_get_lgr(grid.get(), "INNER"));
        }

        THEN("rd_grid_alloc_copy preserves the main grid and LGR names") {
            auto copy =
                rd_grid_ptr(rd_grid_alloc_copy(grid.get()), &rd_grid_free);
            REQUIRE(copy != nullptr);
            REQUIRE(std::string(rd_grid_get_name(copy.get())) ==
                    filename.string());
            REQUIRE(rd_grid_has_lgr(copy.get(), "OUTER"));
            REQUIRE(rd_grid_has_lgr(copy.get(), "INNER"));

            std::string first = rd_grid_iget_lgr_name(copy.get(), 0);
            std::string second = rd_grid_iget_lgr_name(copy.get(), 1);
            REQUIRE((first == "OUTER" || first == "INNER"));
            REQUIRE((second == "OUTER" || second == "INNER"));
            REQUIRE(first != second);
        }
    }
}

TEST_CASE_METHOD(Tmpdir, "Load EGRID with MAPAXES", "[unittest]") {
    GIVEN("A grid with rotated MAPAXES") {
        // MAPAXES = (y_axis_end, origin, x_axis_end) pairs of (x,y).
        // Here the origin is offset to (10, 20) and the axes are rotated
        // by 45 degrees.
        const float mapaxes[6] = {
            10.0f, 21.0f, // y-axis end point
            10.0f, 20.0f, // origin
            11.0f, 21.0f, // x-axis end point
        };
        auto filename = dirname / "MAPAXES.EGRID";
        auto grid = load_egrid_with_single_lgr(filename, 3, 3, 3, 2, 2, 2, 1, 1,
                                               1, "LGR1", mapaxes);

        REQUIRE(grid != nullptr);

        THEN("The grid reports that mapaxes are in use") {
            REQUIRE(rd_grid_use_mapaxes(grid.get()));
        }

        THEN("The regenerated COORD keyword contains inv-transformed "
             "(local-frame) pillar coordinates") {
            auto coord_kw =
                rd_kw_ptr(rd_grid_alloc_coord_kw(grid.get()), &rd_kw_free);
            REQUIRE(coord_kw != nullptr);
            const int nx = rd_grid_get_nx(grid.get());
            const int ny = rd_grid_get_ny(grid.get());
            REQUIRE(rd_kw_get_size(coord_kw.get()) ==
                    RD_GRID_COORD_SIZE(nx, ny));
            const float *data = rd_kw_get_float_ptr(coord_kw.get());
            // Fixture main grid: 3x3x3 unit cells, so pillar (i,j) has
            // local top (i,j,0) and bottom (i,j,3). MAPAXES would offset
            // x by at least the origin (10), so observing x==i, y==j
            // proves the values are in the local (inv-transformed) frame.
            for (int j = 0; j <= ny; j++) {
                for (int i = 0; i <= nx; i++) {
                    int offset = 6 * (j * (nx + 1) + i);
                    REQUIRE(data[offset + 0] == static_cast<float>(i));
                    REQUIRE(data[offset + 1] == static_cast<float>(j));
                    REQUIRE(data[offset + 2] == 0.0f);
                    REQUIRE(data[offset + 3] == static_cast<float>(i));
                    REQUIRE(data[offset + 4] == static_cast<float>(j));
                    REQUIRE(data[offset + 5] == 3.0f);
                }
            }
        }

        THEN("Writing the grid transforms the coords back") {
            auto grid_filename = dirname / "MAPAXES.GRID";
            rd_grid_fwrite_GRID2(grid.get(), grid_filename.c_str(),
                                 RD_METRIC_UNITS);
            REQUIRE(fs::exists(grid_filename));

            fortio_type *fortio = fortio_open_reader(grid_filename.c_str(),
                                                     false, RD_ENDIAN_FLIP);
            REQUIRE(fortio != nullptr);

            rd_kw_ptr first_corners(nullptr, &rd_kw_free);
            while (true) {
                rd_kw_type *kw = rd_kw_fread_alloc(fortio);
                if (kw == nullptr)
                    break;
                if (std::string(rd_kw_get_header(kw)) == CORNERS_KW) {
                    first_corners.reset(kw);
                    break;
                }
                rd_kw_free(kw);
            }
            fortio_fclose(fortio);

            REQUIRE(first_corners != nullptr);
            REQUIRE(rd_kw_get_size(first_corners.get()) == 24);
            const float *corners = rd_kw_get_float_ptr(first_corners.get());
            // Main grid cell (0,0,0) is a unit cell; in the local frame
            // its corners are at the 8 combinations of {0,1} in each axis.
            const float expected[24] = {
                0.0f, 0.0f, 0.0f, // corner 0
                1.0f, 0.0f, 0.0f, // corner 1
                0.0f, 1.0f, 0.0f, // corner 2
                1.0f, 1.0f, 0.0f, // corner 3
                0.0f, 0.0f, 1.0f, // corner 4
                1.0f, 0.0f, 1.0f, // corner 5
                0.0f, 1.0f, 1.0f, // corner 6
                1.0f, 1.0f, 1.0f, // corner 7
            };
            for (int i = 0; i < 24; i++)
                REQUIRE_THAT(corners[i], WithinAbs(expected[i], 0.1f));
        }

        THEN("rd_grid_init_mapaxes_data_double returns mapaxes") {
            double out[6] = {0};
            rd_grid_init_mapaxes_data_double(grid.get(), out);
            for (int i = 0; i < 6; i++)
                REQUIRE_THAT(out[i],
                             WithinAbs(static_cast<double>(mapaxes[i]), 0.1));
        }

        THEN("rd_grid_alloc_mapaxes_kw returns kw with mapaxes") {
            auto kw =
                rd_kw_ptr(rd_grid_alloc_mapaxes_kw(grid.get()), &rd_kw_free);
            REQUIRE(kw != nullptr);
            REQUIRE(rd_kw_get_size(kw.get()) == 6);
            const float *data = rd_kw_get_float_ptr(kw.get());
            for (int i = 0; i < 6; i++)
                REQUIRE(data[i] == mapaxes[i]);
        }

        THEN("rd_grid_alloc_copy preserves the MAPAXES") {
            auto copy =
                rd_grid_ptr(rd_grid_alloc_copy(grid.get()), &rd_grid_free);
            REQUIRE(copy != nullptr);
            REQUIRE(rd_grid_use_mapaxes(copy.get()));

            double original[6] = {0};
            double copied[6] = {0};
            rd_grid_init_mapaxes_data_double(grid.get(), original);
            rd_grid_init_mapaxes_data_double(copy.get(), copied);
            for (int i = 0; i < 6; i++)
                REQUIRE(copied[i] == original[i]);
        }
    }

    GIVEN("An EGRID file with a degenerate MAPAXES whose axes are collinear") {
        const float mapaxes[6] = {
            2.0f, 0.0f, // y-axis end point (collinear with x-axis)
            0.0f, 0.0f, // origin
            1.0f, 0.0f, // x-axis end point
        };
        auto filename = dirname / "MAPAXES_DEGENERATE.EGRID";
        auto grid = load_egrid_with_single_lgr(filename, 2, 2, 2, 2, 2, 2, 0, 0,
                                               0, "LGR1", mapaxes);

        THEN("The grid loads and reports that mapaxes are not in use") {
            REQUIRE(grid != nullptr);
            REQUIRE_FALSE(rd_grid_use_mapaxes(grid.get()));
        }
    }
}

TEST_CASE_METHOD(Tmpdir, "Load GRID file with MAPAXES", "[unittest]") {
    GIVEN("A .GRID file with a MAPAXES keyword in the main grid section") {
        const float mapaxes[6] = {
            10.0f, 21.0f, // y-axis end point
            10.0f, 20.0f, // origin
            11.0f, 21.0f, // x-axis end point
        };
        auto filename = dirname / "MAPAXES.GRID";
        auto grid = load_grid_file_with_mapaxes(filename, mapaxes);

        THEN("The grid loads and reports that mapaxes are in use") {
            REQUIRE(grid != nullptr);
            REQUIRE(rd_grid_use_mapaxes(grid.get()));
        }
    }
}

TEST_CASE_METHOD(Tmpdir, "Load GRID file with an even-nz main grid and an LGR",
                 "[unittest]") {
    // When the main grid's nz is even, the dual-porosity heuristic is
    // entered. If an LGR is present, num_corners exceeds nx*ny*nz and the
    // fracture_index is set to nx*ny*nz/2
    GIVEN("A .GRID file with a 1x1x2 main grid and a 1x1x1 LGR") {
        auto filename = dirname / "EVEN_NZ_WITH_LGR.GRID";
        auto grid = load_grid_file_main_with_lgr(filename, 1, 1, 2);

        THEN("The file loads successfully and is not dual-porosity") {
            REQUIRE(grid != nullptr);
            REQUIRE_FALSE(rd_grid_dual_grid(grid.get()));
            REQUIRE(rd_grid_has_lgr(grid.get(), "LGR1"));
        }
    }
}

TEST_CASE_METHOD(Tmpdir, "Dual-porosity EGRID with NNC is loaded",
                 "[unittest]") {
    const int nx = 1, ny = 1, nz = 4; // 2 matrix + 2 fracture layers
    const int size = nx * ny * nz;
    std::vector<int> actnum(size, CELL_ACTIVE_MATRIX | CELL_ACTIVE_FRACTURE);

    // First NNC connects two matrix cells (valid); the second references a
    // fracture cell index > grid->size (= nx*ny*nz/2 = 2) which is discarded
    const std::vector<int> nnc1 = {1, 3};
    const std::vector<int> nnc2 = {2, 4};

    GIVEN("A dual-porosity EGRID with NNCs referencing fracture cells") {
        auto filename = dirname / "DUALP_NNC.EGRID";
        auto grid = load_egrid_dual_porosity(filename, nx, ny, nz,
                                             actnum.data(), nnc1, nnc2);
        REQUIRE(grid != nullptr);

        THEN("The first NNC is matrix and the second is fracture") {

            // cell 0 has an NNC
            const nnc_info_type *info0 =
                rd_grid_get_cell_nnc_info1(grid.get(), 0);
            REQUIRE(info0 != nullptr);
            REQUIRE(nnc_info_get_total_size(info0) == 1);
            const nnc_vector_type *self0 = nnc_info_get_self_vector(info0);
            REQUIRE(self0 != nullptr);
            REQUIRE(nnc_vector_iget_grid_index(self0, 0) == 1);

            // Cell 1 has no NNC
            const nnc_info_type *info1 =
                rd_grid_get_cell_nnc_info1(grid.get(), 1);
            REQUIRE(info1 == nullptr);
        }
    }
}

TEST_CASE_METHOD(Tmpdir, "Fracture-index queries on a dual-porosity grid",
                 "[unittest]") {
    // Dual porosity gives each cell a matrix-copy and a fracture-copy, so
    // the grid maintains a second active-index for fracture-active
    // cells in addition to the matrix-active one.
    const int nx = 2, ny = 2, nz = 2;
    const int size = nx * ny * nz;

    // Mark every cell as both matrix- and fracture-active, except cell 0
    // which is only matrix-active.
    std::vector<int> actnum(size, CELL_ACTIVE_MATRIX | CELL_ACTIVE_FRACTURE);
    actnum[0] = CELL_ACTIVE_MATRIX;

    auto filename = dirname / "DUALP_FRAC.EGRID";
    auto grid = load_egrid_dual_porosity(filename, nx, ny, nz, actnum.data());

    const int nactive_fracture = rd_grid_get_nactive_fracture(grid.get());

    THEN("The count of fracture-active cells matches the ACTNUM encoding") {
        REQUIRE(nactive_fracture == size - 1);
    }

    THEN("Each fracture-active cell maps to a fracture index") {
        std::vector<bool> seen(nactive_fracture, false);
        for (int g = 0; g < size; ++g) {
            int f = rd_grid_get_active_fracture_index1(grid.get(), g);
            if (actnum[g] & CELL_ACTIVE_FRACTURE) {
                REQUIRE(f >= 0);
                REQUIRE(f < nactive_fracture);
                REQUIRE_FALSE(seen[f]);
                seen[f] = true;
                REQUIRE(rd_grid_get_global_index1F(grid.get(), f) == g);
            } else {
                REQUIRE(f == -1);
            }
        }
        for (bool s : seen)
            REQUIRE(s);
    }

    THEN("A fracture index past the last active returns -1") {
        REQUIRE(rd_grid_get_global_index1F(grid.get(), nactive_fracture) == -1);
    }
}

TEST_CASE_METHOD(Tmpdir, "Load EGRID with amalgamated NNCs between two LGRs",
                 "[unittest]") {
    // NNCHEADA + NNA1 + NNA2 describe NNCs between two different LGRs and
    // drive the amalgamated path rd_grid_init_nnc_amalgamated.
    const std::vector<int> nna1 = {1};
    const std::vector<int> nna2 = {1};

    GIVEN("An EGRID file with two LGRs and an amalgamated NNC section") {
        auto filename = dirname / "LGR_AMALGAMATED_NNC.EGRID";
        auto grid = load_egrid_with_two_lgrs_and_amalgamated_nnc(
            filename, 3, 3, 3, "LGR1", 0, 0, 0, "LGR2", 2, 2, 2, nna1, nna2);

        THEN("The file loads and both LGRs are present") {
            REQUIRE(grid != nullptr);
            REQUIRE(rd_grid_has_lgr(grid.get(), "LGR1"));
            REQUIRE(rd_grid_has_lgr(grid.get(), "LGR2"));
        }
    }
}
