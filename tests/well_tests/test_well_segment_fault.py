from pathlib import Path
from shutil import rmtree

import pytest
from resdata.grid import GridGenerator
from resdata.well import WellInfo, WellSegment

from tests.test_well import (
    IWEL_PRODUCER,
    NX,
    NY,
    NZ,
    Connection,
    Segment,
    Well,
    write_restart,
)


@pytest.fixture(scope="module")
def msw_state():
    workspace = Path("tests/well_tests/well_segment_fault_workspace")
    if workspace.exists():
        rmtree(workspace)
    workspace.mkdir()

    path = workspace / "CASE.X0000"
    grid = GridGenerator.create_rectangular((NX, NY, NZ), (1.0, 1.0, 1.0))
    well = Well(
        name="MSW",
        headi=2,
        headj=3,
        headk=1,
        well_type=IWEL_PRODUCER,
        connections=[
            Connection(2, 3, 1, segment=1),
            Connection(2, 3, 2, segment=2),
            Connection(2, 3, 3, segment=3),
            Connection(2, 3, 4, segment=4),
        ],
        segments=[
            Segment(
                outlet=0,
                branch=1,
                length=12.5,
                diameter=0.25,
                total_length=12.5,
                depth=2500.0,
            ),
            Segment(
                outlet=1,
                branch=1,
                length=20.25,
                diameter=0.30,
                total_length=32.75,
                depth=2515.5,
            ),
            Segment(
                outlet=2,
                branch=1,
                length=7.75,
                diameter=0.35,
                total_length=40.5,
                depth=2522.25,
            ),
            Segment(
                outlet=2,
                branch=2,
                length=5.5,
                diameter=0.20,
                total_length=38.25,
                depth=2518.0,
            ),
        ],
    )
    write_restart(str(path), [well])
    well_info = WellInfo(grid, str(path))

    try:
        yield well_info["MSW"][0]
    finally:
        rmtree(workspace)


def test_segments_are_well_segment_instances(msw_state):
    segments = msw_state.segments()

    assert len(segments) == 4
    assert all(isinstance(segment, WellSegment) for segment in segments)


def test_id_values_are_one_based_integers(msw_state):
    segments = msw_state.segments()

    assert [segment.id() for segment in segments] == [1, 2, 3, 4]
    assert all(type(segment.id()) is int for segment in segments)


def test_link_count_values_are_integers(msw_state):
    segments = msw_state.segments()

    assert [segment.linkCount() for segment in segments] == [1, 1, 0, 0]
    assert all(type(segment.linkCount()) is int for segment in segments)


def test_branch_id_values_distinguish_lateral(msw_state):
    segments = msw_state.segments()

    assert [segment.branchId() for segment in segments] == [1, 1, 1, 2]
    assert all(type(segment.branchId()) is int for segment in segments)


def test_outlet_id_keeps_top_segment_at_zero(msw_state):
    segments = msw_state.segments()

    assert [segment.outletId() for segment in segments] == [0, 1, 2, 2]
    assert all(type(segment.outletId()) is int for segment in segments)


def test_active_returns_true_booleans(msw_state):
    segments = msw_state.segments()

    assert [segment.isActive() for segment in segments] == [True, True, True, True]
    assert all(type(segment.isActive()) is bool for segment in segments)


def test_main_stem_returns_branch_membership_booleans(msw_state):
    segments = msw_state.segments()

    assert [segment.isMainStem() for segment in segments] == [True, True, True, False]
    assert all(type(segment.isMainStem()) is bool for segment in segments)


def test_nearest_wellhead_only_marks_top_segment(msw_state):
    segments = msw_state.segments()

    assert [segment.isNearestWellHead() for segment in segments] == [
        True,
        False,
        False,
        False,
    ]
    assert all(type(segment.isNearestWellHead()) is bool for segment in segments)


def test_depth_and_length_are_written_floats(msw_state):
    segments = msw_state.segments()

    assert [segment.depth() for segment in segments] == [
        2500.0,
        2515.5,
        2522.25,
        2518.0,
    ]
    assert [segment.length() for segment in segments] == [12.5, 20.25, 7.75, 5.5]
    assert all(type(segment.depth()) is float for segment in segments)
    assert all(type(segment.length()) is float for segment in segments)


def test_total_length_and_diameter_are_written_floats(msw_state):
    segments = msw_state.segments()

    assert [segment.totalLength() for segment in segments] == [
        12.5,
        32.75,
        40.5,
        38.25,
    ]
    assert [segment.diameter() for segment in segments] == [0.25, 0.3, 0.35, 0.2]
    assert all(segment.totalLength() > 0 for segment in segments)
    assert all(type(segment.totalLength()) is float for segment in segments)
    assert all(type(segment.diameter()) is float for segment in segments)
