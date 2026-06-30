import datetime
from pathlib import Path

import pytest
from resdata.grid import GridGenerator
from resdata.well import WellInfo, WellState, WellType


@pytest.fixture(scope="module")
def well_time_line():
    base = Path("test-data/local/ECLIPSE/well/missing-ICON")
    grid = GridGenerator.create_rectangular((46, 112, 22), (1, 1, 1))
    well_info = WellInfo(grid, [str(base / "ICON0.X0027"), str(base / "ICON1.X0027")])
    return well_info["B-2H"]


def test_len_matches_number_of_loaded_restart_reports(well_time_line):
    assert len(well_time_line) == 2
    assert isinstance(len(well_time_line), int)


def test_get_name_matches_parent_well_name(well_time_line):
    assert well_time_line.getName() == "B-2H"
    assert "name = B-2H, size = 2" in repr(well_time_line)


def test_first_item_is_public_well_state_with_expected_identity(well_time_line):
    well_state = well_time_line[0]
    assert isinstance(well_state, WellState)
    assert well_state.name() == "B-2H"
    assert well_state.wellNumber() == 1


def test_first_item_preserves_report_time_and_type(well_time_line):
    well_state = well_time_line[0]
    assert well_state.reportNumber() == 27
    assert well_state.simulationTime().datetime() == datetime.datetime(1998, 10, 13)
    assert well_state.wellType() == WellType.PRODUCER


def test_second_item_is_last_loaded_report_with_icon_connections(well_time_line):
    well_state = well_time_line[1]
    assert well_state.name() == "B-2H"
    assert well_state.reportNumber() == 27
    assert well_state.hasGlobalConnections() is True


def test_negative_one_index_returns_last_report(well_time_line):
    well_state = well_time_line[-1]
    assert well_state == well_time_line[1]
    assert well_state.hasGlobalConnections() is True


def test_negative_len_index_returns_first_report(well_time_line):
    well_state = well_time_line[-2]
    assert well_state == well_time_line[0]
    assert well_state.hasGlobalConnections() is False


def test_positive_out_of_range_index_raises_index_error(well_time_line):
    with pytest.raises(IndexError, match=r"Index must be in range 0 <= 2 < 2"):
        well_time_line[2]


def test_too_negative_index_raises_index_error_after_normalization(well_time_line):
    with pytest.raises(IndexError, match=r"Index must be in range 0 <= -1 < 2"):
        well_time_line[-3]


def test_iteration_yields_all_well_states_in_order(well_time_line):
    states = list(well_time_line)
    assert len(states) == 2
    assert all(isinstance(state, WellState) for state in states)
    assert [state.name() for state in states] == ["B-2H", "B-2H"]
    assert [state.hasGlobalConnections() for state in states] == [False, True]
