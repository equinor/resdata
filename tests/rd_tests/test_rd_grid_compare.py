from __future__ import annotations

import pytest

from ._grid_fixtures import (
    CELL_ACTIVE_FRACTURE,
    CELL_ACTIVE_MATRIX,
    generate_coordkw_grid,
    load_egrid_dual_porosity,
    load_egrid_with_coarse_groups,
    load_egrid_with_single_lgr,
    make_rectangular_grid,
)


def _grids_equal(g1, g2) -> bool:
    return g1.equal(g2, include_lgr=True, include_nnc=True)


def test_that_two_rectangular_grids_with_different_actnum_are_unequal():
    g1 = make_rectangular_grid(2, 2, 2, 1, 1, 1, [1] * 8)
    g2 = make_rectangular_grid(2, 2, 2, 1, 1, 1, [1, 0, 1, 1, 1, 1, 1, 1])

    assert not _grids_equal(g1, g2)
    assert _grids_equal(g1, g1)
    assert _grids_equal(g2, g2)


def test_that_two_grids_with_different_cell_corners_are_unequal():
    g1 = generate_coordkw_grid(2, 2, 2)
    g2 = generate_coordkw_grid(2, 2, 2, [(0, 0, 0, 0, 5.0), (1, 1, 1, 7, 42.0)])

    assert not _grids_equal(g1, g2)
    assert _grids_equal(g1, g1)
    assert _grids_equal(g2, g2)


def test_that_two_grids_with_identical_geometry_and_actnum_are_equal():
    g1 = generate_coordkw_grid(2, 2, 2)
    g2 = generate_coordkw_grid(2, 2, 2)

    assert _grids_equal(g1, g2)
    assert _grids_equal(g1, g1)
    assert _grids_equal(g2, g2)


def test_that_egrids_differing_in_coarse_group_assignment_are_unequal(tmp_path):
    nx, ny, nz = 2, 2, 2
    size = nx * ny * nz

    corsnum1 = [0] * size
    corsnum1[0] = 1
    corsnum1[1] = 1

    corsnum2 = [0] * size
    corsnum2[2] = 1
    corsnum2[3] = 1

    g1 = load_egrid_with_coarse_groups(
        tmp_path / "CORSNUM1.EGRID",
        nx,
        ny,
        nz,
        corsnum1,
    )
    g2 = load_egrid_with_coarse_groups(
        tmp_path / "CORSNUM2.EGRID",
        nx,
        ny,
        nz,
        corsnum2,
    )

    assert not _grids_equal(g1, g2)
    assert _grids_equal(g1, g1)
    assert _grids_equal(g2, g2)


def test_that_egrid_with_two_coarse_groups_differs_from_one_with_just_one(tmp_path):
    nx, ny, nz = 2, 2, 2
    size = nx * ny * nz

    corsnum1 = [0] * size
    corsnum1[0] = 1
    corsnum1[1] = 1
    corsnum1[2] = 1
    corsnum1[3] = 1

    corsnum2 = [0] * size
    corsnum2[0] = 1
    corsnum2[1] = 1
    corsnum2[2] = 2
    corsnum2[3] = 2

    g1 = load_egrid_with_coarse_groups(
        tmp_path / "CORSNUM_TWO.EGRID",
        nx,
        ny,
        nz,
        corsnum2,
    )
    g2 = load_egrid_with_coarse_groups(
        tmp_path / "CORSNUM_ONE.EGRID",
        nx,
        ny,
        nz,
        corsnum1,
    )

    assert not _grids_equal(g1, g2)
    assert _grids_equal(g1, g1)
    assert _grids_equal(g2, g2)


def test_that_egrids_with_lgrs_attached_to_different_host_cells_are_unequal(tmp_path):
    g1 = load_egrid_with_single_lgr(
        tmp_path / "LGR_HOST1.EGRID",
        3,
        3,
        3,
        2,
        2,
        2,
        0,
        0,
        0,
        "LGR1",
    )
    g2 = load_egrid_with_single_lgr(
        tmp_path / "LGR_HOST2.EGRID",
        3,
        3,
        3,
        2,
        2,
        2,
        1,
        1,
        1,
        "LGR1",
    )

    assert not _grids_equal(g1, g2)
    assert _grids_equal(g1, g1)
    assert _grids_equal(g2, g2)


def test_that_dual_porosity_grids_differing_in_fracture_actnum_are_unequal(tmp_path):
    nx, ny, nz = 2, 2, 2
    size = nx * ny * nz
    actnum1 = [CELL_ACTIVE_MATRIX | CELL_ACTIVE_FRACTURE] * size
    actnum2 = list(actnum1)
    actnum2[0] = CELL_ACTIVE_MATRIX

    g1 = load_egrid_dual_porosity(
        tmp_path / "DUALP1.EGRID",
        nx,
        ny,
        nz,
        actnum1,
    )
    g2 = load_egrid_dual_porosity(
        tmp_path / "DUALP2.EGRID",
        nx,
        ny,
        nz,
        actnum2,
    )

    assert not _grids_equal(g1, g2)
    assert _grids_equal(g1, g1)
    assert _grids_equal(g2, g2)


def test_that_pairwise_compare_distinguishes_all_representative_grids(all_grids):
    for i, (label_i, gi) in enumerate(all_grids):
        for j, (label_j, gj) in enumerate(all_grids):
            if i == j:
                assert _grids_equal(gi, gj), f"grid {label_i} should be equal to itself"
            else:
                assert not _grids_equal(
                    gi, gj
                ), f"grid {label_i} should differ from {label_j}"
