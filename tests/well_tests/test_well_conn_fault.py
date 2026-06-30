import pytest
from resdata.grid import GridGenerator
from resdata.well import WellConnection, WellConnectionDirection, WellInfo

from tests import ResdataTest


class PathHelper(ResdataTest):
    pass


@pytest.fixture(scope="module")
def well_data():
    helper = PathHelper()
    grid = GridGenerator.create_rectangular((46, 112, 22), (1, 1, 1))
    restart = helper.createTestPath("local/ECLIPSE/well/missing-ICON/ICON1.X0027")
    info = WellInfo(grid, restart)
    return grid, info


@pytest.fixture
def well_state(well_data):
    grid, info = well_data
    return info["B-2H"][0]


@pytest.fixture
def global_connections(well_state):
    return well_state.globalConnections()


def test_constructor_is_not_public():
    with pytest.raises(
        NotImplementedError, match="Class can not be instantiated directly"
    ):
        WellConnection()


def test_well_head_has_exact_zero_based_ijk_tuple(well_state):
    well_head = well_state.wellHead()
    assert well_head.ijk() == (14, 30, 8)
    assert type(well_head.ijk()) is tuple
    assert len(well_head.ijk()) == 3
    assert all(type(value) is int for value in well_head.ijk())


def test_global_connection_count_and_order(global_connections):
    assert [connection.ijk() for connection in global_connections] == [
        (16, 30, 8),
        (18, 30, 8),
        (19, 30, 9),
        (20, 30, 9),
        (21, 31, 9),
        (23, 31, 9),
        (24, 31, 9),
        (28, 32, 9),
    ]


def test_first_global_connection_ijk_component_types(global_connections):
    i, j, k = global_connections[0].ijk()
    assert (i, j, k) == (16, 30, 8)
    assert type(i) is int
    assert type(j) is int
    assert type(k) is int


def test_open_status_is_exact_bool_for_head_and_connections(
    well_state, global_connections
):
    assert well_state.wellHead().isOpen() is True
    assert all(connection.isOpen() is True for connection in global_connections)
    assert all(type(connection.isOpen()) is bool for connection in global_connections)


def test_direction_is_public_enum_value(global_connections):
    directions = [connection.direction() for connection in global_connections]
    assert directions == [WellConnectionDirection.well_conn_dirX] * 8
    assert all(type(direction) is WellConnectionDirection for direction in directions)


def test_segment_id_is_exact_int_sentinel(well_state, global_connections):
    assert well_state.wellHead().segmentId() == -999
    assert type(well_state.wellHead().segmentId()) is int
    assert [connection.segmentId() for connection in global_connections] == [-999] * 8
    assert all(type(connection.segmentId()) is int for connection in global_connections)


def test_fracture_and_matrix_predicates_are_exact_bools(global_connections):
    assert all(
        connection.isFractureConnection() is False for connection in global_connections
    )
    assert all(
        connection.isMatrixConnection() is True for connection in global_connections
    )
    assert all(
        type(connection.isFractureConnection()) is bool
        for connection in global_connections
    )
    assert all(
        type(connection.isMatrixConnection()) is bool
        for connection in global_connections
    )


def test_connection_factor_and_rates_are_exact_floats(well_state):
    well_head = well_state.wellHead()
    assert well_head.connectionFactor() == -1.0
    assert type(well_head.connectionFactor()) is float
    assert well_head.gasRate() == 0.0
    assert well_head.waterRate() == 0.0
    assert well_head.oilRate() == 0.0
    assert well_head.volumeRate() == 0.0
    assert well_head.gasRateSI() == 0.0
    assert well_head.waterRateSI() == 0.0
    assert well_head.oilRateSI() == 0.0
    assert well_head.volumeRateSI() == 0.0
    assert all(
        type(value) is float
        for value in (
            well_head.gasRate(),
            well_head.waterRate(),
            well_head.oilRate(),
            well_head.volumeRate(),
            well_head.gasRateSI(),
            well_head.waterRateSI(),
            well_head.oilRateSI(),
            well_head.volumeRateSI(),
        )
    )


def test_equality_and_repr_use_public_connection_state(global_connections):
    first = global_connections[0]
    second = global_connections[1]
    assert first == first
    assert first != second
    text = repr(first)
    assert "WellConnection((16, 30, 8) open " in text
    assert "rates = (O:0.0,G:0.0,W:0.0)" in text
    assert "direction = well_conn_dirX" in text
