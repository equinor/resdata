from __future__ import annotations

from pathlib import Path

import pytest

from resdata import ResDataType
from resdata.grid import Grid
from resdata.resfile import ResdataKW

from ._grid_fixtures import (
    make_rectangular_grid,
    generate_coordkw_grid,
    write_fegrid_minimal,
)


@pytest.mark.parametrize("global_index", [0, 1, 10, 20])
def test_that_unfractured_grid_returns_minus_one_on_fracture_index(global_index):
    grid = make_rectangular_grid(21, 11, 12, 1, 2, 3)
    assert grid.get_num_active_fracture() == 0
    assert grid.get_active_fracture_index(global_index=global_index) == -1


def test_that_grdecl_kw_with_explicit_actnum_creates_grid_with_correct_active_size():
    nx, ny, nz = 2, 2, 2
    actnum = [1] * 8
    actnum[0] = 0
    grid = generate_coordkw_grid(nx, ny, nz, actnum=actnum)
    assert grid is not None
    assert grid.get_num_active() == nx * ny * nz - 1


def test_that_writing_egrid_and_loading_back_gives_equal_grid(tmp_path):
    grid = make_rectangular_grid(5, 5, 5, 1, 1, 1)
    filename = tmp_path / "CASE.EGRID"
    grid.save_EGRID(str(filename))

    loaded = Grid(str(filename))
    assert grid.equal(loaded, include_lgr=False, include_nnc=False)


def test_that_writing_egrid_with_unknown_extension_still_succeeds(tmp_path):
    grid = make_rectangular_grid(5, 5, 5, 1, 1, 1)
    filename = tmp_path / "CASE.UNKNOWN"
    grid.save_EGRID(str(filename))
    assert filename.exists()
    assert filename.stat().st_size > 0


@pytest.fixture
def regular_grid_2x2x2():
    return make_rectangular_grid(2, 2, 2, 1.0, 2.0, 3.0, [1, 1, 1, 1, 0, 1, 1, 1])


def test_that_regular_grid_reports_correct_dimensions(regular_grid_2x2x2):
    grid = regular_grid_2x2x2
    assert grid.get_nx() == 2
    assert grid.get_ny() == 2
    assert grid.get_nz() == 2

    nx, ny, nz, nactive = grid.get_dims()
    assert (nx, ny, nz) == (2, 2, 2)
    assert nactive == 7

    assert grid.get_global_size() == 8
    assert grid.get_num_active() == 7


def test_that_regular_grid_translates_indices_consistently(regular_grid_2x2x2):
    grid = regular_grid_2x2x2

    global_idx = grid.get_global_index(ijk=(0, 0, 0))
    assert global_idx == 0

    assert grid.get_ijk(global_index=global_idx) == (0, 0, 0)

    active_idx = grid.get_active_index(ijk=(0, 0, 0))
    assert active_idx == 0
    assert grid.get_active_index(global_index=global_idx) == 0
    assert grid.global_index(active_index=active_idx) == global_idx
    assert grid.get_ijk(active_index=active_idx) == (0, 0, 0)


def test_that_invalid_ijk_is_detected(regular_grid_2x2x2):
    grid = regular_grid_2x2x2
    assert not grid.cell_invalid(ijk=(0, 0, 0))
    assert not grid.cell_invalid(ijk=(1, 1, 1))


def test_that_active_status_is_reported_per_cell(regular_grid_2x2x2):
    grid = regular_grid_2x2x2
    assert grid.active(ijk=(0, 0, 0))
    assert grid.active(global_index=0)
    assert not grid.active(global_index=4)


def test_that_xyz_returns_cell_centroid(regular_grid_2x2x2):
    grid = regular_grid_2x2x2

    assert grid.get_xyz(ijk=(0, 0, 0)) == (0.5, 1.0, 1.5)
    assert grid.get_xyz(global_index=1) == (1.5, 1.0, 1.5)

    active_idx = grid.get_active_index(global_index=6)
    assert grid.get_xyz(active_index=active_idx) == (0.5, 3.0, 4.5)


def test_that_cell_corner_returns_corner_coordinates(regular_grid_2x2x2):
    grid = regular_grid_2x2x2
    assert grid.get_cell_corner(0, global_index=0) == (0.0, 0.0, 0.0)
    assert grid.get_cell_corner(3, global_index=0) == (1.0, 2.0, 0.0)


def test_that_cell_dims_match_input_spacing(regular_grid_2x2x2):
    grid = regular_grid_2x2x2
    dx, dy, dz = grid.get_cell_dims(global_index=0)
    assert (dx, dy, dz) == (1.0, 2.0, 3.0)
    assert grid.cell_dz(global_index=0) == 3.0


def test_that_cell_volume_equals_dx_times_dy_times_dz(regular_grid_2x2x2):
    grid = regular_grid_2x2x2
    assert grid.cell_volume(global_index=0) == 6.0
    active_idx = grid.get_active_index(global_index=0)
    assert grid.cell_volume(active_index=active_idx) == 6.0


def test_that_depth_top_and_bottom_match_geometry(regular_grid_2x2x2):
    grid = regular_grid_2x2x2
    assert grid.depth(global_index=0) == 1.5
    active_idx = grid.get_active_index(global_index=0)
    assert grid.depth(active_index=active_idx) == 1.5
    assert grid.top(0, 0) == 0.0
    assert grid.bottom(0, 0) == 6.0
    assert grid.locate_depth(1.5, 0, 0) == 0


def test_that_cell_contains_its_own_centroid(regular_grid_2x2x2):
    grid = regular_grid_2x2x2
    x, y, z = grid.get_xyz(global_index=0)
    assert grid.cell_contains(x, y, z, global_index=0)


def test_that_regular_grid_has_no_lgrs(regular_grid_2x2x2):
    grid = regular_grid_2x2x2
    assert grid.get_num_lgr() == 0
    assert not grid.has_lgr("test")


def test_that_regular_grid_has_no_coarse_cells(regular_grid_2x2x2):
    grid = regular_grid_2x2x2
    assert grid.coarse_groups() == 0
    assert not grid.in_coarse_group(global_index=0)


def test_that_regular_grid_has_no_fracture_cells(regular_grid_2x2x2):
    grid = regular_grid_2x2x2
    assert grid.get_num_active_fracture() == 0
    assert grid.get_active_fracture_index(global_index=0) == -1
    assert grid.get_global_index1F(0) is None


def test_that_zcorn_export_has_expected_size_and_layered_values(regular_grid_2x2x2):
    grid = regular_grid_2x2x2
    zcorn = list(grid.export_zcorn())
    assert len(zcorn) == 2 * 2 * 2 * 8

    expected = [0.0] * 16 + [3.0] * 32 + [6.0] * 16
    assert zcorn == expected


def test_that_coord_export_matches_corner_pillars(regular_grid_2x2x2):
    grid = regular_grid_2x2x2
    expected = [
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        6.0,
        1.0,
        0.0,
        0.0,
        1.0,
        0.0,
        6.0,
        2.0,
        0.0,
        0.0,
        2.0,
        0.0,
        6.0,
        0.0,
        2.0,
        0.0,
        0.0,
        2.0,
        6.0,
        1.0,
        2.0,
        0.0,
        1.0,
        2.0,
        6.0,
        2.0,
        2.0,
        0.0,
        2.0,
        2.0,
        6.0,
        0.0,
        4.0,
        0.0,
        0.0,
        4.0,
        6.0,
        1.0,
        4.0,
        0.0,
        1.0,
        4.0,
        6.0,
        2.0,
        4.0,
        0.0,
        2.0,
        4.0,
        6.0,
    ]
    assert list(grid.export_coord()) == expected


def test_that_actnum_export_matches_input(regular_grid_2x2x2):
    grid = regular_grid_2x2x2
    actnum_data = [1, 1, 1, 1, 0, 1, 1, 1]
    assert list(grid.export_actnum()) == actnum_data


def test_that_export_actnum_kw_has_correct_size_and_values(regular_grid_2x2x2):
    grid = regular_grid_2x2x2
    actnum_data = [1, 1, 1, 1, 0, 1, 1, 1]
    actnum_kw = grid.export_ACTNUM_kw()
    assert len(actnum_kw) == 8
    assert [actnum_kw[i] for i in range(len(actnum_kw))] == actnum_data


def test_that_volume_keyword_matches_cell_volume(regular_grid_2x2x2):
    grid = regular_grid_2x2x2
    volume_kw = grid.create_volume_keyword(active_size=False)
    assert len(volume_kw) == 8
    for i in range(len(volume_kw)):
        assert volume_kw[i] == 6.0

    volume_kw_active = grid.create_volume_keyword(active_size=True)
    assert len(volume_kw_active) == 7
    for i in range(len(volume_kw_active)):
        assert volume_kw_active[i] == 6.0


def test_that_export_index_matches_documented_layout(regular_grid_2x2x2):
    grid = regular_grid_2x2x2
    df = grid.export_index()
    rows = df.values.tolist()
    expected = [
        [0, 0, 0, 0],
        [1, 0, 0, 1],
        [0, 1, 0, 2],
        [1, 1, 0, 3],
        [0, 0, 1, -1],
        [1, 0, 1, 4],
        [0, 1, 1, 5],
        [1, 1, 1, 6],
    ]
    assert rows == expected


def test_that_export_volume_returns_six_for_each_cell(regular_grid_2x2x2):
    grid = regular_grid_2x2x2
    df = grid.export_index()
    volume = grid.export_volume(df)
    assert len(volume) == 8
    for v in volume:
        assert v == 6.0


def test_that_export_position_returns_centroids(regular_grid_2x2x2):
    grid = regular_grid_2x2x2
    df = grid.export_index()
    positions = grid.export_position(df)
    expected = [
        [0.5, 1.0, 1.5],
        [1.5, 1.0, 1.5],
        [0.5, 3.0, 1.5],
        [1.5, 3.0, 1.5],
        [0.5, 1.0, 4.5],
        [1.5, 1.0, 4.5],
        [0.5, 3.0, 4.5],
        [1.5, 3.0, 4.5],
    ]
    assert positions.tolist() == expected


def test_that_int_keyword_can_be_exported_with_global_index_subset(regular_grid_2x2x2):
    grid = regular_grid_2x2x2
    pvtnum = ResdataKW("PVTNUM", grid.get_global_size(), ResDataType.RD_INT)
    for i in range(grid.get_global_size()):
        pvtnum[i] = 1

    df = grid.export_index().iloc[:3]
    out = grid.export_data(df, pvtnum)
    assert list(out) == [1, 1, 1]


def test_that_float_keyword_can_be_exported_with_global_index_subset(
    regular_grid_2x2x2,
):
    grid = regular_grid_2x2x2
    nactive = grid.get_num_active()
    poro = ResdataKW("PORO", nactive, ResDataType.RD_FLOAT)
    for a in range(nactive):
        poro[a] = 0.2 + a * 0.01

    df = grid.export_index().iloc[:3]
    out = grid.export_data(df, poro)
    assert out[0] == pytest.approx(0.20, abs=1e-4)
    assert out[1] == pytest.approx(0.21, abs=1e-4)
    assert out[2] == pytest.approx(0.22, abs=1e-4)


@pytest.fixture
def load_case_setup(tmp_path):
    grid_5x5x5 = make_rectangular_grid(5, 5, 5, 1, 1, 1)
    grid_2x2x2 = make_rectangular_grid(2, 2, 2, 1, 1, 1)
    grid_3x3x3 = make_rectangular_grid(3, 3, 3, 1, 1, 1)

    grid_5x5x5.save_EGRID(str(tmp_path / "CASE.EGRID"))
    grid_2x2x2.save_GRID(str(tmp_path / "CASE.GRID"))
    grid_3x3x3.save_GRID(str(tmp_path / "ONLYGRID.GRID"))
    write_fegrid_minimal(tmp_path / "ONLYFEGRID.FEGRID")

    return {
        "grid_5x5x5": grid_5x5x5,
        "grid_2x2x2": grid_2x2x2,
        "grid_3x3x3": grid_3x3x3,
    }


def _load_and_assert_dims(path: Path, expected: Grid) -> None:
    loaded = Grid.load_from_file(str(path))
    assert loaded is not None
    assert loaded.get_dims()[:3] == expected.get_dims()[:3]


def test_that_load_from_file_picks_up_explicit_egrid(load_case_setup, tmp_path):
    _load_and_assert_dims(
        tmp_path / "CASE.EGRID",
        load_case_setup["grid_5x5x5"],
    )


def test_that_load_from_file_picks_up_explicit_grid_file(load_case_setup, tmp_path):
    _load_and_assert_dims(
        tmp_path / "CASE.GRID",
        load_case_setup["grid_2x2x2"],
    )


def test_that_load_from_file_picks_up_grid_file_when_only_grid_exists(
    load_case_setup, tmp_path
):
    _load_and_assert_dims(
        tmp_path / "ONLYGRID.GRID",
        load_case_setup["grid_3x3x3"],
    )
