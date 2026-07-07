"""Tests for the ``resdata.well`` Python bindings.

The specification for the restart file format can be found
in [OPM flow 2025.04 manual Appendix F.8](https://opm-project.org/?page_id=955)

"""

import datetime
import os
from collections.abc import Iterable
from dataclasses import dataclass, field
from typing import TypeAlias

import hypothesis.strategies as st
import pytest
from hypothesis import given
from resdata import ResDataType
from resdata.grid import GridGenerator
from resdata.resfile import FortIO, ResdataKW
from resdata.resfile.rd_file import ResdataFile
from resdata.well import (
    WellConnectionDirection,
    WellInfo,
    WellSegment,
    WellState,
    WellTimeLine,
    WellType,
)

# These are the per-array sizes that is stored in the INTEHEAD
NIWELZ = 105  # Number of values per well in IWEL
NZWELZ = 3  # 8-character words per well in ZWEL
NXWELZ = 8  # Number of values per well in XWEL
NICONZ = 25  # Number of values per connection in ICON
NSCONZ = 25  # Number of values per connection in SCON
NXCONZ = 60  # Number of values per connection in XCON
NISEGZ = 22  # Number of values per segment in ISEG
NRSEGZ = 140  # Number of values per segment in RSEG
NSWLMX = 2  # max segmented wells
NLBRMX = 2  # max lateral branches per well
NILBRZ = 10  # Number of values per branch in ILBR

# Grid dimensions used both for the INTEHEAD keyword and the generated grid.
NX, NY, NZ = 20, 20, 10

# IWEL indices
IWEL_HEADI, IWEL_HEADJ, IWEL_HEADK = 0, 1, 2
IWEL_CONNECTIONS = 4
IWEL_TYPE = 6
IWEL_STATUS = 10
IWEL_SEGMENTED_WELL_NR = 70

# ICON indices.
ICON_IC, ICON_I, ICON_J, ICON_K = 0, 1, 2, 3
ICON_STATUS = 5
ICON_DIRECTION = 13
ICON_SEGMENT = 14

# XCON indices (per-connection rates).
XCON_ORAT, XCON_WRAT, XCON_GRAT = 0, 1, 2
XCON_QR = 49  # reservoir volume rate

# ISEG indices.
ISEG_OUTLET = 1
ISEG_BRANCH = 3

# RSEG indices.
RSEG_LENGTH = 0
RSEG_DIAMETER = 2
RSEG_TOTAL_LENGTH = 6
RSEG_DEPTH = 7

# XWEL items (reservoir rates).
XWEL_ORAT, XWEL_WRAT, XWEL_GRAT, XWEL_RESV = 0, 1, 2, 4

# IWEL well-type values, matching the WellType enum.
IWEL_PRODUCER = 1
IWEL_OIL_INJECTOR = 2
IWEL_WATER_INJECTOR = 3
IWEL_GAS_INJECTOR = 4

# A connection direction value of 0 in ICON is interpreted as "Z".
ICON_DIR_DEFAULT = 0

# Unit systems, as stored in INTEHEAD[2] and matching the ert_rd_unit_enum.
METRIC_UNITS = 1
FIELD_UNITS = 2
LAB_UNITS = 3

# Unit conversion constants
INCH = 0.0254  # meters
FEET = 12 * INCH
GALLON = 231 * INCH**3
BARREL = GALLON * 42
LITER = 0.001  # m^3
MILLI_LITER = LITER * 0.001
MMCF = (FEET**3) * 1_000_000
HOUR = 3600  # seconds
DAY = 24 * HOUR

# Factor that converts a raw oil/water rate to SI (rm3/s or sm3/s) per unit system.
LIQUID_RATE_SI_FACTOR = {
    METRIC_UNITS: 1.0 / DAY,
    FIELD_UNITS: BARREL / DAY,
    LAB_UNITS: MILLI_LITER / HOUR,
}

# Gas uses a different reservoir volume unit in field units.
GAS_RATE_SI_FACTOR = {
    METRIC_UNITS: 1.0 / DAY,
    FIELD_UNITS: MMCF / DAY,
    LAB_UNITS: MILLI_LITER / HOUR,
}


@dataclass
class Connection:
    i: int
    j: int
    k: int
    status: int = 1
    direction: int = ICON_DIR_DEFAULT
    segment: int = 0
    rates: tuple | None = None  # (oil, water, gas, resv) connection rates


@dataclass
class Segment:
    outlet: int
    branch: int
    length: float = 0.0
    diameter: float = 0.0
    total_length: float = 0.0
    depth: float = 0.0


@dataclass
class Well:
    name: str
    headi: int = 1
    headj: int = 1
    headk: int = 1
    well_type: int = IWEL_PRODUCER
    status: int = 1
    connections: list = field(default_factory=list)
    segments: list = field(default_factory=list)
    rates: tuple | None = None  # (water, gas, oil, resv) reservoir rates


def _int_kw(name, size):
    kw = ResdataKW(name, size, ResDataType.RD_INT)
    for i in range(size):
        kw[i] = 0
    return kw


def _double_kw(name, size):
    kw = ResdataKW(name, size, ResDataType.RD_DOUBLE)
    for i in range(size):
        kw[i] = 0.0
    return kw


def _float_kw(name, size):
    kw = ResdataKW(name, size, ResDataType.RD_FLOAT)
    for i in range(size):
        kw[i] = 0.0
    return kw


def _bool_kw(name, size):
    kw = ResdataKW(name, size, ResDataType.RD_BOOL)
    for i in range(size):
        kw[i] = False
    return kw


def _char_kw(name, size):
    kw = ResdataKW(name, size, ResDataType.RD_CHAR)
    for i in range(size):
        kw[i] = " " * 8
    return kw


Year: TypeAlias = int
Month: TypeAlias = int
Day: TypeAlias = int
Date: TypeAlias = tuple[Year, Month, Day]


# LOGIHEAD index that flags a dual-porosity run.
LOGIHEAD_DUALP = 14


def _logihead_kw(dualp=False):
    kw = _bool_kw("LOGIHEAD", 30)
    kw[LOGIHEAD_DUALP] = dualp
    return kw


def _step_keywords(
    wells: list[Well],
    date: Date,
    seqnum: int | None = None,
    include_icon: bool = True,
    unit_system: int = METRIC_UNITS,
    dualp: bool = False,
):
    ncwmax = max(len(w.connections) for w in wells)
    nsegmx = max(len(w.segments) for w in wells)

    def _icon_kw(wells):
        kw = _int_kw("ICON", NICONZ * ncwmax * len(wells))
        for i, well in enumerate(wells):
            for j, conn in enumerate(well.connections):
                off = NICONZ * (ncwmax * i + j)
                kw[off + ICON_IC] = j + 1
                kw[off + ICON_I] = conn.i
                kw[off + ICON_J] = conn.j
                kw[off + ICON_K] = conn.k
                kw[off + ICON_STATUS] = conn.status
                kw[off + ICON_DIRECTION] = conn.direction
                kw[off + ICON_SEGMENT] = conn.segment
        return kw

    def _intehead_kw(
        year,
        month,
        day,
        unit_system=METRIC_UNITS,
    ):
        kw = _int_kw("INTEHEAD", 412)
        kw[2] = unit_system
        kw[8], kw[9], kw[10] = NX, NY, NZ
        kw[11] = NX * NY * NZ  # nactive
        kw[14] = 7  # oil + gas + water
        kw[16] = 0  # nwells, filled in by caller
        kw[17] = ncwmax
        kw[24] = NIWELZ
        kw[26] = NXWELZ
        kw[27] = NZWELZ
        kw[32] = NICONZ
        kw[33] = NSCONZ
        kw[34] = NXCONZ
        kw[64], kw[65], kw[66] = day, month, year
        kw[94] = 100  # IPROG / simulator version
        kw[175] = NSWLMX
        kw[176] = nsegmx
        kw[177] = NLBRMX
        kw[178] = NISEGZ
        kw[179] = NRSEGZ
        kw[180] = NILBRZ
        return kw

    def _iwel_kw(wells: list[Well]) -> ResdataKW:
        kw = _int_kw("IWEL", NIWELZ * len(wells))
        for i, well in enumerate(wells):
            off = NIWELZ * i
            kw[off + IWEL_HEADI] = well.headi
            kw[off + IWEL_HEADJ] = well.headj
            kw[off + IWEL_HEADK] = well.headk
            kw[off + IWEL_CONNECTIONS] = len(well.connections)
            kw[off + IWEL_TYPE] = well.well_type
            kw[off + IWEL_STATUS] = well.status
            # A value of -1 (==0 after the implicit -1 offset) marks a non-segmented
            # well; any other value flags the well as multi-segmented.
            kw[off + IWEL_SEGMENTED_WELL_NR] = i + 1 if well.segments else -1
        return kw

    def _zwel_kw(wells):
        kw = _char_kw("ZWEL", NZWELZ * len(wells))
        for i, well in enumerate(wells):
            kw[NZWELZ * i] = well.name.ljust(8)
        return kw

    def _scon_kw(wells):
        kw = _float_kw("SCON", NSCONZ * ncwmax * len(wells))
        for i, well in enumerate(wells):
            for j, _ in enumerate(well.connections):
                off = NSCONZ * (ncwmax * i + j)
                kw[off + 0] = float(j + 1)  # connection factor
        return kw

    def _iseg_kw(wells):
        kw = _int_kw("ISEG", NISEGZ * nsegmx * len(wells))
        for i, well in enumerate(wells):
            for seg_index, segment in enumerate(well.segments):
                off = NISEGZ * (nsegmx * i + seg_index)
                kw[off + ISEG_OUTLET] = segment.outlet
                kw[off + ISEG_BRANCH] = segment.branch
        return kw

    def _rseg_kw(wells):
        # RSEG is read into a double buffer by the C rseg-loader, so it must be
        # written with double precision for the indexed read to line up.
        kw = _double_kw("RSEG", NRSEGZ * nsegmx * len(wells))
        for i, well in enumerate(wells):
            for seg_index, segment in enumerate(well.segments):
                off = NRSEGZ * (nsegmx * i + seg_index)
                kw[off + RSEG_LENGTH] = segment.length
                kw[off + RSEG_DIAMETER] = segment.diameter
                kw[off + RSEG_TOTAL_LENGTH] = segment.total_length
                kw[off + RSEG_DEPTH] = segment.depth
        return kw

    def _xwel_kw(wells):
        kw = _double_kw("XWEL", NXWELZ * len(wells))
        for i, well in enumerate(wells):
            if well.rates is None:
                continue
            water, gas, oil, resv = well.rates
            off = NXWELZ * i
            kw[off + XWEL_WRAT] = water
            kw[off + XWEL_GRAT] = gas
            kw[off + XWEL_ORAT] = oil
            kw[off + XWEL_RESV] = resv
        return kw

    def _xcon_kw(wells):
        kw = _double_kw("XCON", NXCONZ * ncwmax * len(wells))
        for i, well in enumerate(wells):
            for j, conn in enumerate(well.connections):
                if conn.rates is None:
                    continue
                oil, water, gas, resv = conn.rates
                off = NXCONZ * (ncwmax * i + j)
                kw[off + XCON_ORAT] = oil
                kw[off + XCON_WRAT] = water
                kw[off + XCON_GRAT] = gas
                kw[off + XCON_QR] = resv
        return kw

    year, month, day = date
    intehead = _intehead_kw(year, month, day, unit_system=unit_system)
    intehead[16] = len(wells)

    keywords = []
    if seqnum is not None:
        seqnum_kw = _int_kw("SEQNUM", 1)
        seqnum_kw[0] = seqnum
        keywords.append(seqnum_kw)

    keywords += [
        intehead,
        _logihead_kw(dualp=dualp),
        _double_kw("DOUBHEAD", 50),
        _iwel_kw(wells),
        _zwel_kw(wells),
    ]
    # Some restart files (e.g. certain E300 outputs) omit the ICON keyword
    # altogether. In that case the well still has a wellhead from IWEL, but no
    # global connections, and well_state.hasGlobalConnections() returns False.
    if include_icon:
        keywords.append(_icon_kw(wells))
        keywords.append(_scon_kw(wells))
        if any(conn.rates is not None for well in wells for conn in well.connections):
            keywords.append(_xcon_kw(wells))
    if any(well.rates is not None for well in wells):
        keywords.append(_xwel_kw(wells))
    if any(well.segments for well in wells):
        keywords.append(_iseg_kw(wells))
        keywords.append(_rseg_kw(wells))
    return keywords


def _fwrite_keywords(path, keywords):
    scratch = path + ".scratch"
    fortio = FortIO(scratch, mode=FortIO.WRITE_MODE)
    for kw in keywords:
        kw.fwrite(fortio)
    fortio.close()

    rd_file = ResdataFile(scratch)
    fortio = FortIO(path, mode=FortIO.WRITE_MODE)
    rd_file.fwrite(fortio)
    fortio.close()
    os.remove(scratch)


def write_restart(
    path,
    wells: list[Well],
    date: Date = (2020, 1, 1),
    include_icon: bool = True,
    unit_system: int = METRIC_UNITS,
    dualp: bool = False,
):
    """Write a non-unified restart file (``.X#### ``) with a single report step."""
    _fwrite_keywords(
        path,
        _step_keywords(
            wells,
            date,
            include_icon=include_icon,
            unit_system=unit_system,
            dualp=dualp,
        ),
    )


def write_unified_restart(path, steps: Iterable[tuple[int, Date, list[Well]]]):
    """Write a unified restart file (``.UNRST``)."""
    keywords = []
    for report_idx, date, wells in steps:
        keywords += _step_keywords(wells, date, seqnum=report_idx)
    _fwrite_keywords(path, keywords)


@pytest.fixture
def grid():
    return GridGenerator.create_rectangular((NX, NY, NZ), (1.0, 1.0, 1.0))


@pytest.fixture
def producer(grid, tmp_path):
    def inner(*args, **kwargs):
        well = Well(
            name="OP1",
            headi=2,
            headj=3,
            headk=1,
            well_type=IWEL_PRODUCER,
            status=1,
            connections=[
                Connection(i=2, j=3, k=1),
                Connection(i=2, j=3, k=2),
            ],
        )
        path = str(tmp_path / "CASE.X0000")
        write_restart(path, [well], *args, **kwargs)

        return WellInfo(grid, path)

    return inner


def test_that_a_single_producer_well_is_loaded_with_correct_metadata(producer):
    well_info = producer()

    assert len(well_info) == 1
    assert "OP1" in well_info
    assert well_info.allWellNames() == ["OP1"]

    well_state = well_info["OP1"][0]
    assert well_state.name() == "OP1"
    assert well_state.wellType() == WellType.PRODUCER
    assert well_state.isOpen()
    assert well_state.wellNumber() == 0
    assert well_state.hasGlobalConnections()


@pytest.mark.parametrize(
    "iwel_type, expected_type",
    [
        (IWEL_PRODUCER, WellType.PRODUCER),
        (IWEL_OIL_INJECTOR, WellType.OIL_INJECTOR),
        (IWEL_WATER_INJECTOR, WellType.WATER_INJECTOR),
        (IWEL_GAS_INJECTOR, WellType.GAS_INJECTOR),
    ],
)
def test_that_the_well_type_is_read_from_iwel(tmp_path, grid, iwel_type, expected_type):
    well = Well(name="W1", well_type=iwel_type, connections=[Connection(1, 1, 1)])
    path = str(tmp_path / "CASE.X0000")
    write_restart(path, [well])

    well_state = WellInfo(grid, path)["W1"][0]

    assert well_state.wellType() == expected_type


def test_that_status_0_means_closed_well(tmp_path, grid):
    well = Well(
        name="SHUT",
        well_type=IWEL_PRODUCER,
        status=0,
        connections=[Connection(1, 1, 1)],
    )
    path = str(tmp_path / "CASE.X0000")
    write_restart(path, [well])

    well_state = WellInfo(grid, path)["SHUT"][0]

    assert not well_state.isOpen()


def test_that_well_connections_are_loaded_with_ijk_and_status(producer):
    connections = producer()["OP1"][0].globalConnections()

    assert len(connections) == 2
    # ICON indices are 1-based; the bindings return 0-based ijk.
    assert connections[0].ijk() == (1, 2, 0)
    assert connections[1].ijk() == (1, 2, 1)
    assert all(conn.isOpen() for conn in connections)
    assert all(conn.isMatrixConnection() for conn in connections)
    assert all(not conn.isFractureConnection() for conn in connections)


def test_that_a_well_without_an_icon_keyword_has_no_global_connections(producer):
    # Some restart files (e.g. certain E300 outputs) lack the ICON keyword.
    # The well is still present with a wellhead, but exposes no global
    # connections.
    well_state = producer(include_icon=False)["OP1"][0]

    assert not well_state.hasGlobalConnections()
    assert well_state.globalConnections() == []


def test_that_a_well_without_an_icon_keyword_still_has_metadata_and_wellhead(producer):
    well_state = producer(include_icon=False)["OP1"][0]

    assert well_state.name() == "OP1"
    assert well_state.wellType() == WellType.PRODUCER
    assert well_state.isOpen()
    # The wellhead is taken from IWEL and is therefore available even without
    # the ICON keyword.
    assert well_state.wellHead().ijk() == (1, 2, 0)


def test_that_a_well_with_an_icon_keyword_has_global_connections(producer):
    well_state = producer(include_icon=True)["OP1"][0]

    assert well_state.hasGlobalConnections()
    assert len(well_state.globalConnections()) == 2


def test_that_a_well_with_an_empty_icon_keyword_has_zero_global_connections(
    tmp_path, grid
):
    # A well with no perforations still writes an (all-zero) ICON keyword, so
    # the global connection collection exists but is empty. This is distinct
    # from a file that omits ICON entirely.
    well = Well(name="DRY", well_type=IWEL_PRODUCER, connections=[])
    path = str(tmp_path / "CASE.X0000")
    write_restart(path, [well], include_icon=True)

    well_state = WellInfo(grid, path)["DRY"][0]

    assert well_state.hasGlobalConnections()
    assert well_state.globalConnections() == []


@pytest.mark.parametrize(
    "icon_direction, expected_direction",
    [
        (1, WellConnectionDirection.well_conn_dirX),
        (2, WellConnectionDirection.well_conn_dirY),
        (3, WellConnectionDirection.well_conn_dirZ),
        (ICON_DIR_DEFAULT, WellConnectionDirection.well_conn_dirZ),
    ],
)
def test_that_connection_direction_is_read_from_icon(
    tmp_path, grid, icon_direction, expected_direction
):
    well = Well(
        name="W1",
        connections=[Connection(i=1, j=1, k=1, direction=icon_direction)],
    )
    path = str(tmp_path / "CASE.X0000")
    write_restart(path, [well])

    connection = WellInfo(grid, path)["W1"][0].globalConnections()[0]

    assert connection.direction() == expected_direction


@pytest.mark.parametrize(
    "icon_direction, expected_direction",
    [
        (4, WellConnectionDirection.well_conn_fracX),
        (5, WellConnectionDirection.well_conn_fracY),
    ],
)
def test_that_fracture_connection_direction_is_read_from_icon(
    tmp_path, grid, icon_direction, expected_direction
):
    # Fractured directions (fracX/fracY) are only valid for non-matrix
    # connections, so a dual-porosity file with a connection in the fracture
    # part of the grid (k >= nz/2) is required.
    well = Well(
        name="W1",
        connections=[Connection(i=1, j=1, k=NZ // 2 + 1, direction=icon_direction)],
    )
    path = str(tmp_path / "CASE.X0000")
    write_restart(path, [well], dualp=True)

    connection = WellInfo(grid, path)["W1"][0].globalConnections()[0]

    assert connection.isFractureConnection()
    assert connection.direction() == expected_direction


def test_that_a_connection_with_an_out_of_range_direction_is_dropped(
    tmp_path, grid, capfd
):
    # A direction value greater than the largest valid ICON direction
    # (ICON_FRACY == 5) will drop the connection from the list and
    # print to stderr.
    well = Well(
        name="W1",
        connections=[
            Connection(i=1, j=1, k=1),
            Connection(i=2, j=2, k=2, direction=6),
        ],
    )
    path = str(tmp_path / "CASE.X0000")
    write_restart(path, [well])

    connections = WellInfo(grid, path)["W1"][0].globalConnections()

    assert len(connections) == 1
    assert connections[0].ijk() == (0, 0, 0)
    assert "Invalid direction value:6" in capfd.readouterr().err


def test_that_a_matrix_connection_with_a_fracture_direction_is_dropped(
    tmp_path, grid, capfd
):
    # fracX/fracY are only valid for fracture (non-matrix) connections. Setting
    # a fracture direction on a matrix connection will drop it from the list
    # of connections and print to stderr
    well = Well(
        name="W1",
        connections=[
            Connection(i=1, j=1, k=1),
            Connection(i=2, j=2, k=2, direction=4),
        ],
    )
    path = str(tmp_path / "CASE.X0000")
    write_restart(path, [well])

    connections = WellInfo(grid, path)["W1"][0].globalConnections()

    assert len(connections) == 1
    assert connections[0].ijk() == (0, 0, 0)
    assert "invalid direction" in capfd.readouterr().err


def test_that_connection_factor_is_read_from_scon(producer):
    connections = producer()["OP1"][0].globalConnections()

    assert connections[0].connectionFactor() == 1.0
    assert connections[1].connectionFactor() == 2.0


def test_that_well_head_returns_the_global_connection_head(producer):
    well_head = producer()["OP1"][0].wellHead()

    # IWEL stores the head as (2, 3, 1) using 1-based indices.
    assert well_head.ijk() == (1, 2, 0)
    assert well_head.isOpen()


def test_that_the_well_head_direction_is_z(producer):
    well_head = producer()["OP1"][0].wellHead()

    # The wellhead is a matrix connection. Its direction is
    # always vertical (Z).
    assert well_head.isMatrixConnection()
    assert well_head.direction() == WellConnectionDirection.well_conn_dirZ
    assert well_head.isOpen()


def test_that_the_fracture_well_head_direction_is_z(tmp_path, grid):
    well = Well(
        name="OP1",
        headi=2,
        headj=3,
        headk=NZ // 2 + 1,
        well_type=IWEL_PRODUCER,
        connections=[Connection(i=2, j=3, k=NZ // 2 + 1)],
    )
    path = str(tmp_path / "CASE.X0000")
    write_restart(path, [well], dualp=True)

    well_head = WellInfo(grid, path)["OP1"][0].wellHead()

    assert well_head.isFractureConnection()
    assert well_head.direction() == WellConnectionDirection.well_conn_dirZ
    assert well_head.isOpen()


def test_that_the_well_head_reports_the_cell_where_the_well_enters(tmp_path, grid):
    well = Well(
        name="OP1",
        headi=7,
        headj=4,
        headk=2,
        well_type=IWEL_PRODUCER,
        connections=[Connection(7, 4, 2)],
    )
    path = str(tmp_path / "CASE.X0000")
    write_restart(path, [well])

    head = WellInfo(grid, path)["OP1"][0].wellHead()

    assert head.ijk() == (6, 3, 1)


def test_that_the_well_head_reports_the_cell_after_the_well_lookup_is_discarded(
    tmp_path, grid
):
    well = Well(
        name="OP1",
        headi=7,
        headj=4,
        headk=2,
        well_type=IWEL_PRODUCER,
        connections=[Connection(7, 4, 2)],
    )
    path = str(tmp_path / "CASE.X0000")
    write_restart(path, [well])

    head = WellInfo(grid, path)["OP1"][0].wellHead()

    assert head.ijk() == (6, 3, 1)
    assert head.isOpen()


def test_that_multiple_wells_are_all_available_by_name_and_index(tmp_path, grid):
    wells = [
        Well(name="PROD", well_type=IWEL_PRODUCER, connections=[Connection(1, 1, 1)]),
        Well(
            name="INJ",
            well_type=IWEL_WATER_INJECTOR,
            connections=[Connection(5, 5, 1)],
        ),
    ]
    path = str(tmp_path / "CASE.X0000")
    write_restart(path, wells)

    well_info = WellInfo(grid, path)

    assert len(well_info) == 2
    assert set(well_info.allWellNames()) == {"PROD", "INJ"}
    assert well_info.hasWell("PROD")
    assert well_info.hasWell("INJ")
    assert [time_line.getName() for time_line in well_info] == ["PROD", "INJ"]


def test_that_querying_an_unknown_well_raises_keyerror(producer):
    well_info = producer()
    assert "NOPE" not in well_info
    with pytest.raises(KeyError):
        _ = well_info["NOPE"]


def test_that_indexing_a_well_out_of_range_raises_indexerror(producer):
    well_info = producer()
    with pytest.raises(IndexError):
        _ = well_info[5]


def test_that_a_unified_restart_builds_a_time_line_with_all_report_steps(
    tmp_path, grid
):
    well = Well(
        name="OP1",
        well_type=IWEL_PRODUCER,
        connections=[Connection(1, 1, 1)],
    )

    steps = [
        (0, (2020, 1, 1), [well]),
        (1, (2021, 1, 1), [well]),
        (2, (2022, 1, 1), [well]),
    ]
    path = str(tmp_path / "CASE.UNRST")
    write_unified_restart(path, steps)

    time_line = WellInfo(grid, path)["OP1"]

    assert repr(time_line).startswith("WellTimeLine(name = OP1, size = 3)")

    assert len(time_line) == 3
    assert (
        time_line[-1].simulationTime() == time_line[len(time_line) - 1].simulationTime()
    )
    assert [state.reportNumber() for state in time_line] == [0, 1, 2]
    assert [state.simulationTime().datetime().year for state in time_line] == [
        2020,
        2021,
        2022,
    ]


def test_that_simulation_time_matches_the_restart_report_date(producer):
    well_state = producer(date=(2020, 6, 29))["OP1"][0]

    sim_date = well_state.simulationTime().datetime()
    assert (sim_date.year, sim_date.month, sim_date.day) == (2020, 6, 29)


def test_that_states_are_in_the_order_of_the_file(tmp_path, grid):
    well = Well(name="OP1", well_type=IWEL_PRODUCER, connections=[Connection(1, 1, 1)])
    steps = [
        (0, (2020, 1, 1), [well]),
        (2, (2022, 1, 1), [well]),
        (1, (2021, 1, 1), [well]),
    ]
    path = str(tmp_path / "CASE.UNRST")
    write_unified_restart(path, steps)

    times = [state.simulationTime().datetime() for state in WellInfo(grid, path)["OP1"]]

    assert times == [
        datetime.datetime(2020, 1, 1, 0, 0),
        datetime.datetime(2022, 1, 1, 0, 0),
        datetime.datetime(2021, 1, 1, 0, 0),
    ]


def test_that_each_iwel_type_code_maps_to_the_expected_well_type(tmp_path, grid):
    wells = [
        Well(name="PROD", well_type=IWEL_PRODUCER, connections=[Connection(1, 1, 1)]),
        Well(
            name="OIL", well_type=IWEL_OIL_INJECTOR, connections=[Connection(2, 1, 1)]
        ),
        Well(
            name="WAT", well_type=IWEL_WATER_INJECTOR, connections=[Connection(3, 1, 1)]
        ),
        Well(
            name="GAS", well_type=IWEL_GAS_INJECTOR, connections=[Connection(4, 1, 1)]
        ),
    ]
    path = str(tmp_path / "CASE.X0000")
    write_restart(path, wells)

    well_info = WellInfo(grid, path)
    assert well_info["PROD"][0].wellType() == WellType.PRODUCER
    assert well_info["OIL"][0].wellType() == WellType.OIL_INJECTOR
    assert well_info["WAT"][0].wellType() == WellType.WATER_INJECTOR
    assert well_info["GAS"][0].wellType() == WellType.GAS_INJECTOR


def test_that_a_multisegment_well_exposes_its_segments(tmp_path, grid):
    well = Well(
        name="MSW",
        headi=2,
        headj=3,
        headk=1,
        well_type=IWEL_PRODUCER,
        connections=[
            Connection(i=2, j=3, k=1, segment=1),
            Connection(i=2, j=3, k=2, segment=2),
        ],
        segments=[
            Segment(outlet=0, branch=1, length=10.0, total_length=10.0, depth=100.0),
            Segment(outlet=1, branch=1, length=20.0, total_length=30.0, depth=120.0),
            Segment(outlet=2, branch=1, length=15.0, total_length=45.0, depth=135.0),
        ],
    )
    path = str(tmp_path / "CASE.X0000")
    write_restart(path, [well])

    well_state = WellInfo(grid, path)["MSW"][0]

    assert well_state.isMultiSegmentWell()
    assert well_state.hasSegmentData()
    assert well_state.numSegments() == 3
    assert len(well_state) == 3
    assert well_state.igetSegment(0) == well_state[0]

    segments = well_state.segments()
    assert [segment.id() for segment in segments] == [
        well_state[i].id() for i in range(len(well_state))
    ]
    assert well_state[-1].id() == well_state[len(well_state) - 1].id()
    assert well_state.name() == "MSW"
    assert repr(well_state).startswith(
        'WellState(MSW (multi segment), number = 0, type = "PRODUCER", state = open)'
    )
    assert [segment.id() for segment in segments] == [1, 2, 3]
    assert all(segment.branchId() == 1 for segment in segments)
    assert all(segment.isMainStem() for segment in segments)
    # The first segment is closest to the wellhead (outlet == 0).
    assert segments[0].isNearestWellHead()
    assert segments[1].outletId() == 1


@pytest.fixture
def multi_segment_well(grid, tmp_path):
    well = Well(
        name="MSW",
        headi=2,
        headj=3,
        headk=1,
        well_type=IWEL_PRODUCER,
        connections=[
            Connection(i=2, j=3, k=1, segment=1),
            Connection(i=2, j=3, k=2, segment=2),
        ],
        segments=[
            Segment(outlet=0, branch=1, length=10.0, total_length=10.0, depth=100.0),
            Segment(outlet=1, branch=1, length=20.0, total_length=30.0, depth=120.0),
        ],
    )
    path = str(tmp_path / "CASE.X0000")
    write_restart(path, [well])
    return WellInfo(grid, path)


def test_that_a_multisegment_wells_tubing_geometry_is_reported_as_written(
    multi_segment_well,
):
    segments = multi_segment_well["MSW"][0].segments()

    assert [seg.id() for seg in segments] == [1, 2]
    assert [seg.length() for seg in segments] == [10.0, 20.0]
    assert [seg.depth() for seg in segments] == [100.0, 120.0]


def test_that_segment_indexing_matches_the_segments_list(multi_segment_well):
    well_state = multi_segment_well["MSW"][0]
    segments = well_state.segments()

    assert [well_state[i].id() for i in range(len(well_state))] == [
        seg.id() for seg in segments
    ]


def test_that_negative_segment_index_returns_the_last_segment(multi_segment_well):
    well_state = multi_segment_well["MSW"][0]

    assert well_state[-1].id() == well_state.segments()[-1].id()


def test_that_segment_branch_and_outlet_match_what_was_written(multi_segment_well):
    segments = multi_segment_well["MSW"][0].segments()

    assert all(seg.branchId() == 1 for seg in segments)
    assert segments[1].outletId() == 1


def test_that_connections_of_a_multisegment_well_are_multisegment(multi_segment_well):
    connections = multi_segment_well["MSW"][0].globalConnections()

    assert len(connections) == 2
    assert all(conn.isMultiSegmentWell() for conn in connections)


def test_that_segment_id_differs_from_outlet_id(multi_segment_well):
    segments = multi_segment_well["MSW"][0].segments()

    assert all(segment.id() != segment.outletId() for segment in segments)


def test_that_segment_geometry_is_read_from_rseg(tmp_path, grid):
    well = Well(
        name="MSW",
        well_type=IWEL_PRODUCER,
        connections=[Connection(i=1, j=1, k=1, segment=1)],
        segments=[
            Segment(
                outlet=0,
                branch=1,
                length=12.5,
                diameter=0.25,
                total_length=12.5,
                depth=2500.0,
            ),
        ],
    )
    path = str(tmp_path / "CASE.X0000")
    write_restart(path, [well])

    segment = WellInfo(grid, path)["MSW"][0].segments()[0]

    assert segment.length() == 12.5
    assert segment.diameter() == 0.25
    assert segment.totalLength() == 12.5
    assert segment.depth() == 2500.0
    assert segment.linkCount() == 0
    assert segment.isActive()
    assert repr(segment).startswith(
        "WellSegment({Segment ID:1   BranchID:1  Length:12.5})"
    )
    assert str(segment) == "{Segment ID:1   BranchID:1  Length:12.5}"


@st.composite
def segment_trees(draw):
    """Generate a random multi-segment well as a list of ``Segment`` slots.

    A segment's *id* is the 1-based position of its slot in the ISEG array, so a
    random subset of the slots is chosen to hold active segments (the segment
    indices) and the remaining slots are filled with inactive placeholders
    (``branch == 0``).

    The active segments form a tree rooted at the nearest wellhead (``outlet == 0``).
    Segments are grouped into branches (a branch is a chain of segments sharing
    a branch id).
    """
    n = draw(st.integers(min_value=1, max_value=40))
    outlet_node = [None] * n
    branch_of_node = [0] * n
    branch_of_node[0] = 1
    branch_tail = {1: 0}  # branch id -> the last segment currently on that branch
    next_branch = 2
    for node in range(1, n):
        if draw(st.booleans()):
            # Start a new lateral branch off any already-created segment.
            outlet_node[node] = draw(st.integers(min_value=0, max_value=node - 1))
            branch_of_node[node] = next_branch
            branch_tail[next_branch] = node
            next_branch += 1
        else:
            # Extend an existing branch: outlet is that branch's current tail.
            extended = draw(st.sampled_from(sorted(branch_tail)))
            outlet_node[node] = branch_tail[extended]
            branch_of_node[node] = extended
            branch_tail[extended] = node

    # Assign each node a unique 1-based ISEG position. Drawing from a range wider
    # than ``n`` leaves gaps (inactive slots) between the active segments.
    id_span = draw(st.integers(min_value=n, max_value=2 * n))
    id_of_node = draw(
        st.lists(
            st.integers(min_value=1, max_value=id_span),
            min_size=n,
            max_size=n,
            unique=True,
        )
    )

    slots = [Segment(outlet=0, branch=0) for _ in range(max(id_of_node))]
    for node in range(n):
        outlet_id = 0 if outlet_node[node] is None else id_of_node[outlet_node[node]]
        slots[id_of_node[node] - 1] = Segment(
            outlet=outlet_id, branch=branch_of_node[node]
        )
    return slots


@given(segment_trees())
def test_that_segment_tree_is_read_from_iseg(tmp_path_factory, slots):
    grid = GridGenerator.create_rectangular((NX, NY, NZ), (1.0, 1.0, 1.0))
    tmp_path = tmp_path_factory.mktemp("segment_tree")

    # The expected active segments in ISEG-position order (== ascending id).
    expected = [
        (index + 1, segment)
        for index, segment in enumerate(slots)
        if segment.branch != 0
    ]
    root_id = next(seg_id for seg_id, segment in expected if segment.outlet == 0)

    well = Well(
        name="MSW",
        well_type=IWEL_PRODUCER,
        connections=[Connection(i=1, j=1, k=1, segment=root_id)],
        segments=slots,
    )
    path = str(tmp_path / "CASE.X0000")
    write_restart(path, [well])

    read_segments = WellInfo(grid, path)["MSW"][0].segments()

    # The segments must be read back in the expected (ascending id) order.
    assert [segment.id() for segment in read_segments] == [
        seg_id for seg_id, _ in expected
    ]

    for read_segment, (seg_id, segment) in zip(read_segments, expected):
        assert read_segment.id() == seg_id
        assert read_segment.branchId() == segment.branch
        assert read_segment.outletId() == segment.outlet
        assert read_segment.isNearestWellHead() == (segment.outlet == 0)

        # A segment's link count is the number of child segments that continue
        # in the same branch, i.e. reference it as their outlet.
        expected_link_count = sum(
            1
            for _, child in expected
            if child.outlet == seg_id and child.branch == segment.branch
        )
        assert read_segment.linkCount() == expected_link_count


def test_that_a_non_segmented_well_has_no_segments(producer):
    well_state = producer()["OP1"][0]

    assert not well_state.isMultiSegmentWell()
    assert not well_state.hasSegmentData()
    assert well_state.numSegments() == 0
    assert well_state.segments() == []
    assert len(well_state) == 0


def test_that_connections_of_a_non_segmented_well_are_not_multisegment(producer):
    connections = producer()["OP1"][0].globalConnections()

    assert len(connections) == 2
    assert all(not conn.isMultiSegmentWell() for conn in connections)


def test_that_disabling_segment_loading_skips_segment_data(tmp_path, grid):
    well = Well(
        name="MSW",
        well_type=IWEL_PRODUCER,
        connections=[Connection(i=1, j=1, k=1, segment=1)],
        segments=[Segment(outlet=0, branch=1, length=10.0)],
    )
    path = str(tmp_path / "CASE.X0000")
    write_restart(path, [well])

    well_info = WellInfo(grid, path, load_segment_information=False)
    well_state = well_info["MSW"][0]

    assert not well_state.hasSegmentData()
    assert well_state.numSegments() == 0


def test_that_a_well_without_rate_data_reports_zero_rates(producer):
    well_state = producer()["OP1"][0]

    assert well_state.oilRate() == 0
    assert well_state.gasRate() == 0
    assert well_state.waterRate() == 0
    assert well_state.volumeRate() == 0
    assert well_state.oilRateSI() == 0
    assert well_state.gasRateSI() == 0
    assert well_state.waterRateSI() == 0
    assert well_state.volumeRateSI() == 0


@pytest.mark.parametrize("unit_system", [METRIC_UNITS, FIELD_UNITS, LAB_UNITS])
def test_that_well_state_si_rates_are_raw_rates_scaled_by_the_unit_factor(
    tmp_path, grid, unit_system
):
    raw_water, raw_gas, raw_oil, raw_resv = 10.0, 20.0, 30.0, 40.0
    well = Well(
        name="OP1",
        well_type=IWEL_PRODUCER,
        connections=[Connection(1, 1, 1)],
        rates=(raw_water, raw_gas, raw_oil, raw_resv),
    )
    path = str(tmp_path / "CASE.X0000")
    write_restart(path, [well], unit_system=unit_system)

    well_state = WellInfo(grid, path)["OP1"][0]

    assert well_state.waterRate() == raw_water
    assert well_state.gasRate() == raw_gas
    assert well_state.oilRate() == raw_oil
    assert well_state.volumeRate() == raw_resv

    assert well_state.oilRateSI() == pytest.approx(
        raw_oil * LIQUID_RATE_SI_FACTOR[unit_system]
    )
    assert well_state.waterRateSI() == pytest.approx(
        raw_water * LIQUID_RATE_SI_FACTOR[unit_system]
    )
    assert well_state.gasRateSI() == pytest.approx(
        raw_gas * GAS_RATE_SI_FACTOR[unit_system]
    )
    assert well_state.volumeRateSI() == pytest.approx(
        raw_resv * LIQUID_RATE_SI_FACTOR[unit_system]
    )


def test_that_connection_si_rates_are_scaled_by_the_unit_factor(tmp_path, grid):
    well = Well(
        name="OP1",
        well_type=IWEL_PRODUCER,
        connections=[
            Connection(1, 1, 1, rates=(1.0, 2.0, 3.0, 4.0)),  # oil, water, gas, resv
            Connection(1, 1, 2, rates=(5.0, 6.0, 7.0, 8.0)),
        ],
    )
    path = str(tmp_path / "CASE.X0000")
    write_restart(path, [well], unit_system=FIELD_UNITS)

    connections = WellInfo(grid, path)["OP1"][0].globalConnections()

    assert len(connections) == 2
    for connection, (oil, water, gas, resv) in zip(
        connections, [(1.0, 2.0, 3.0, 4.0), (5.0, 6.0, 7.0, 8.0)]
    ):
        assert connection.oilRate() == oil
        assert connection.waterRate() == water
        assert connection.gasRate() == gas
        assert connection.volumeRate() == resv

        assert connection.oilRateSI() == pytest.approx(
            oil * LIQUID_RATE_SI_FACTOR[FIELD_UNITS]
        )
        assert connection.waterRateSI() == pytest.approx(
            water * LIQUID_RATE_SI_FACTOR[FIELD_UNITS]
        )
        assert connection.gasRateSI() == pytest.approx(
            gas * GAS_RATE_SI_FACTOR[FIELD_UNITS]
        )
        assert connection.volumeRateSI() == pytest.approx(
            resv * LIQUID_RATE_SI_FACTOR[FIELD_UNITS]
        )


def test_that_well_connection_ijk_match_the_perforations_written_to_the_restart(
    tmp_path, grid
):
    well = Well(
        name="OP1",
        well_type=IWEL_PRODUCER,
        connections=[Connection(3, 4, 5), Connection(6, 7, 8)],
    )
    path = str(tmp_path / "CASE.X0000")
    write_restart(path, [well])

    connections = WellInfo(grid, path)["OP1"][0].globalConnections()

    assert [conn.ijk() for conn in connections] == [(2, 3, 4), (5, 6, 7)]


def test_that_two_calls_to_global_connections_return_equal_ijk_lists(producer):
    well_state = producer()["OP1"][0]

    first = [conn.ijk() for conn in well_state.globalConnections()]
    second = [conn.ijk() for conn in well_state.globalConnections()]
    assert first == second


def test_that_connections_preserve_icon_ordering(tmp_path, grid):
    well = Well(
        name="OP1",
        well_type=IWEL_PRODUCER,
        connections=[Connection(1, 1, k) for k in range(1, 5)],
    )
    path = str(tmp_path / "CASE.X0000")
    write_restart(path, [well])

    connections = WellInfo(grid, path)["OP1"][0].globalConnections()

    assert [conn.ijk() for conn in connections] == [(0, 0, k) for k in range(4)]


def test_that_global_connections_of_two_wells_have_different_ijk(tmp_path, grid):
    wells = [
        Well(name="PROD", well_type=IWEL_PRODUCER, connections=[Connection(1, 1, 1)]),
        Well(
            name="INJ", well_type=IWEL_WATER_INJECTOR, connections=[Connection(5, 5, 1)]
        ),
    ]
    path = str(tmp_path / "CASE.X0000")
    write_restart(path, wells)

    well_info = WellInfo(grid, path)
    assert [c.ijk() for c in well_info["PROD"][0].globalConnections()] == [(0, 0, 0)]
    assert [c.ijk() for c in well_info["INJ"][0].globalConnections()] == [(4, 4, 0)]


connection_idx = st.integers(min_value=1, max_value=6)
connections = st.builds(Connection, *([connection_idx] * 3))


@given(st.lists(connections))
def test_that_all_written_connections_are_read(tmp_path_factory, connections):
    grid = GridGenerator.create_rectangular((NX, NY, NZ), (1.0, 1.0, 1.0))
    tmp_path = tmp_path_factory.mktemp("write_connections")
    well = Well(
        name="OP1",
        well_type=IWEL_PRODUCER,
        connections=connections,
    )
    path = str(tmp_path / "CASE.X0000")
    write_restart(path, [well])

    assert {
        conn.ijk() for conn in WellInfo(grid, path)["OP1"][0].globalConnections()
    } == {(c.i - 1, c.j - 1, c.k - 1) for c in connections}


def test_that_querying_one_well_does_not_change_another_wells_connections(
    tmp_path, grid
):
    wells = [
        Well(name="PROD", well_type=IWEL_PRODUCER, connections=[Connection(1, 1, 1)]),
        Well(
            name="INJ", well_type=IWEL_WATER_INJECTOR, connections=[Connection(5, 5, 1)]
        ),
    ]
    path = str(tmp_path / "CASE.X0000")
    write_restart(path, wells)

    well_info = WellInfo(grid, path)
    inj_first = [c.ijk() for c in well_info["INJ"][0].globalConnections()]
    _ = well_info["PROD"][0].globalConnections()
    inj_second = [c.ijk() for c in well_info["INJ"][0].globalConnections()]
    assert inj_first == inj_second


def test_that_connection_factor_and_rates_match_per_connection(tmp_path, grid):
    well = Well(
        name="OP1",
        well_type=IWEL_PRODUCER,
        connections=[
            Connection(1, 1, 1, rates=(1.0, 2.0, 3.0, 4.0)),
            Connection(1, 1, 2, rates=(5.0, 6.0, 7.0, 8.0)),
        ],
    )
    path = str(tmp_path / "CASE.X0000")
    write_restart(path, [well])

    connections = WellInfo(grid, path)["OP1"][0].globalConnections()

    assert connections[0].oilRate() == 1.0
    assert connections[1].oilRate() == 5.0


def test_that_connections_load_with_their_connection_factor(producer):
    connections = producer()["OP1"][0].globalConnections()

    assert len(connections) == 2
    assert [conn.connectionFactor() for conn in connections] == [1.0, 2.0]


def test_that_segment_connections_are_consistent_with_global_connections(
    multi_segment_well,
):
    connections = multi_segment_well["MSW"][0].globalConnections()

    assert len(connections) == 2


def test_that_reopening_the_same_restart_yields_consistent_connection_count(producer):
    counts = [len(producer()["OP1"][0].globalConnections()) for _ in range(5)]
    assert counts == [2] * 5


def test_that_a_restart_file_can_be_loaded_from_a_resdatafile_instance(producer):
    well_info = producer()

    assert "OP1" in well_info
    assert well_info["OP1"][0].wellType() == WellType.PRODUCER


def test_that_an_unknown_rate_data_argument_type_is_rejected(grid):
    well_info = WellInfo(grid)

    with pytest.raises(TypeError):
        well_info.addWellFile(42, load_segment_information=True)


def test_that_well_state_cannot_directly_be_constructed():
    with pytest.raises(NotImplementedError):
        WellState()


def test_that_well_segment_cannot_directly_be_constructed():
    with pytest.raises(NotImplementedError):
        WellSegment()


def test_that_well_timeline_cannot_directly_be_constructed():
    with pytest.raises(NotImplementedError):
        WellTimeLine()


def test_that_non_existent_well_file_raises_os_error(grid, tmp_path):
    well_info = WellInfo(grid)
    with pytest.raises(OSError, match="No such file"):
        well_info.addWellFile(str(tmp_path / "DOES_NOT_EXIST"), False)
