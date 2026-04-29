from __future__ import annotations

import pytest

from ._grid_fixtures import load_egrid_with_coarse_groups


@pytest.fixture
def two_groups_grid(tmp_path):
    nx, ny, nz = 3, 3, 3
    size = nx * ny * nz
    corsnum = [0] * size
    corsnum[0] = 1
    corsnum[1] = 1
    corsnum[size - 1] = 2
    corsnum[size - 2] = 2
    return load_egrid_with_coarse_groups(
        tmp_path / "CORSNUM.EGRID",
        nx,
        ny,
        nz,
        corsnum,
    )


def test_that_grid_with_two_coarse_groups_reports_coarse_cells_present(two_groups_grid):
    assert two_groups_grid.coarse_groups() == 2


def test_that_cells_in_a_coarse_group_are_flagged_as_such(two_groups_grid):
    size = two_groups_grid.get_global_size()
    assert two_groups_grid.in_coarse_group(global_index=0)
    assert two_groups_grid.in_coarse_group(global_index=1)
    assert not two_groups_grid.in_coarse_group(global_index=2)
    assert two_groups_grid.in_coarse_group(global_index=size - 1)
    assert two_groups_grid.in_coarse_group(global_index=size - 2)


def test_that_grid_with_two_coarse_groups_reports_no_fracture_cells(two_groups_grid):
    size = two_groups_grid.get_global_size()
    assert two_groups_grid.get_active_fracture_index(global_index=0) == -1
    assert two_groups_grid.get_active_fracture_index(global_index=size // 2) == -1
    assert two_groups_grid.get_global_index1F(0) is None


def test_that_actnum_export_matches_per_cell_active_status(two_groups_grid):
    actnum = list(two_groups_grid.export_actnum())
    for i in range(two_groups_grid.get_global_size()):
        expected = 1 if two_groups_grid.active(global_index=i) else 0
        assert actnum[i] == expected


def test_that_non_consecutive_coarse_group_numbers_report_highest_group(tmp_path):
    # CORSNUM uses groups 1 and 3 but not 2. The grid reports the
    # highest group number even though group 2 is unused.
    nx, ny, nz = 3, 3, 3
    size = nx * ny * nz
    corsnum = [0] * size
    corsnum[0] = 1
    corsnum[1] = 1
    corsnum[size - 1] = 3
    corsnum[size - 2] = 3

    grid = load_egrid_with_coarse_groups(
        tmp_path / "CORSNUM_SPARSE.EGRID",
        nx,
        ny,
        nz,
        corsnum,
    )
    assert grid.coarse_groups() == 3
