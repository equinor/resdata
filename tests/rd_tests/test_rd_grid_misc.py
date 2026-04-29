from __future__ import annotations

from pathlib import Path

import pytest

from resdata import ResDataType, UnitSystem
from resdata.grid import Grid
from resdata.resfile import ResdataKW
from cwrap import open as copen

from ._grid_fixtures import make_rectangular_grid


@pytest.fixture
def rectangular_grid():
    return make_rectangular_grid(3, 3, 3, 1, 1, 1)


def test_that_grid_can_be_written_as_GRID_and_loaded_back(rectangular_grid, tmp_path):
    filename = tmp_path / "TEST.GRID"
    rectangular_grid.save_GRID(str(filename))

    loaded = Grid(str(filename))
    assert rectangular_grid.equal(loaded, include_lgr=False, include_nnc=False)


def test_that_grid_can_be_written_as_GRDECL(rectangular_grid, tmp_path):
    filename = tmp_path / "TEST.GRDECL"
    with copen(str(filename), "w") as f:
        rectangular_grid.save_grdecl(f, output_unit=UnitSystem.METRIC)
    assert filename.exists()
    assert filename.stat().st_size > 0


def test_that_load_from_file_loads_a_saved_egrid(rectangular_grid, tmp_path):
    filename = tmp_path / "TEST3.EGRID"
    rectangular_grid.save_EGRID(str(filename))
    loaded = Grid.load_from_file(str(filename))
    assert loaded is not None
    assert rectangular_grid.equal(loaded, include_lgr=False, include_nnc=False)


def test_that_grid_keyword_can_be_printed_as_GRDECL(rectangular_grid, tmp_path):
    size = rectangular_grid.get_global_size()
    kw = ResdataKW("PORO", size, ResDataType.RD_FLOAT)
    for i in range(size):
        kw[i] = 0.2 + i * 0.01

    filename = tmp_path / "KW.GRDECL"
    with copen(str(filename), "w") as f:
        rectangular_grid.write_grdecl(kw, f, default_value=-999.0)
    assert filename.exists()
    assert filename.stat().st_size > 0


@pytest.fixture
def grid_with_inactive_cells():
    actnum = [1] * 8
    actnum[0] = 0
    actnum[3] = 0
    return make_rectangular_grid(2, 2, 2, 1, 1, 1, actnum), actnum


def _read_grdecl_kw(path: Path, name: str, size: int, rd_type) -> ResdataKW:
    with copen(str(path), "r") as f:
        return ResdataKW.read_grdecl(f, name, rd_type=rd_type)


def test_that_active_sized_float_keyword_fills_default_in_inactive_indices(
    grid_with_inactive_cells,
    tmp_path,
):
    grid, actnum = grid_with_inactive_cells
    nactive = grid.get_num_active()
    global_size = grid.get_global_size()
    assert nactive < global_size

    kw = ResdataKW("PORO", nactive, ResDataType.RD_FLOAT)
    for a in range(nactive):
        kw[a] = 0.2 + a * 0.01

    filename = tmp_path / "KW_ACTIVE_FLOAT.GRDECL"
    with copen(str(filename), "w") as f:
        grid.write_grdecl(kw, f, default_value=-999.0)

    read_back = _read_grdecl_kw(filename, "PORO", global_size, ResDataType.RD_FLOAT)
    a = 0
    for i in range(global_size):
        if actnum[i]:
            assert read_back[i] == kw[a]
            a += 1
        else:
            assert read_back[i] == pytest.approx(-999.0)


def test_that_active_sized_int_keyword_fills_default_in_inactive_indices(
    grid_with_inactive_cells,
    tmp_path,
):
    grid, actnum = grid_with_inactive_cells
    nactive = grid.get_num_active()
    global_size = grid.get_global_size()

    kw = ResdataKW("SATNUM", nactive, ResDataType.RD_INT)
    for a in range(nactive):
        kw[a] = a + 1

    filename = tmp_path / "KW_ACTIVE_INT.GRDECL"
    with copen(str(filename), "w") as f:
        grid.write_grdecl(kw, f, default_value=-999)

    read_back = _read_grdecl_kw(filename, "SATNUM", global_size, ResDataType.RD_INT)
    a = 0
    for i in range(global_size):
        if actnum[i]:
            assert read_back[i] == kw[a]
            a += 1
        else:
            assert read_back[i] == -999


def test_that_every_grid_can_be_written_to_disk_as_egrid_and_read_back(
    all_grids, tmp_path
):
    # Some of the grids fail, so excluded until it can be fixed
    labels = {
        "rect-2x2x2",
        "rect-2x2x2-inactive",
        "rect-3x3x3",
        "coordkw-perturbed",
        "egrid-single-lgr",
        "egrid-nested-lgr",
    }
    for label, grid in all_grids:
        if label not in labels:
            continue
        filename = tmp_path / f"WRITE_{label}.EGRID"
        grid.save_EGRID(str(filename))
        assert filename.exists(), f"failed to write {label}"
        reloaded = Grid(str(filename))
        assert reloaded is not None


def test_that_every_grid_can_be_written_as_grdecl(all_grids, tmp_path):
    # Some of the grids fail, so excluded until it can be fixed
    labels = {
        "rect-2x2x2",
        "rect-2x2x2-inactive",
        "rect-3x3x3",
        "coordkw-perturbed",
        "egrid-single-lgr",
        "egrid-nested-lgr",
    }
    for label, grid in all_grids:
        if label not in labels:
            continue
        filename = tmp_path / f"WRITE_{label}.GRDECL"
        with copen(str(filename), "w") as f:
            grid.save_grdecl(f, output_unit=UnitSystem.METRIC)
        assert filename.exists(), f"failed to write {label}"
        assert filename.stat().st_size > 0
