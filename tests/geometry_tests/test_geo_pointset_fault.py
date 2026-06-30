from pathlib import Path

import pytest
from resdata.geometry import GeoPointset, Surface

SURFACE_PATH = (
    Path(__file__).resolve().parents[2]
    / "test-data"
    / "local"
    / "geometry"
    / "surface"
    / "valid_ascii.irap"
)


def load_pointset():
    surface = Surface(str(SURFACE_PATH))
    return surface, GeoPointset.fromSurface(surface)


def test_direct_constructed_pointset_is_empty_with_repr_len():
    pointset = GeoPointset()

    assert len(pointset) == 0
    assert repr(pointset).startswith("GeoPointset(len=0)")


def test_two_empty_pointsets_compare_equal():
    assert GeoPointset() == GeoPointset()


def test_empty_pointset_rejects_index_zero():
    with pytest.raises(
        IndexError, match=r"Invalid index, must be in \[0, 0\), was: 0\."
    ):
        GeoPointset()[0]


def test_surface_pointset_has_exact_surface_length():
    surface, pointset = load_pointset()

    assert len(surface) == 49 * 79
    assert len(pointset) == 3871
    assert len(pointset) == len(surface)


def test_from_surface_static_method_matches_surface_get_pointset():
    surface = Surface(str(SURFACE_PATH))

    assert GeoPointset.fromSurface(surface) == surface.getPointset()


def test_getitem_returns_float_z_values_at_known_indices():
    surface, pointset = load_pointset()

    for index, expected in ((0, 0.0051), (1, 0.0017), (561, 0.0095), (3870, -0.0014)):
        value = pointset[index]
        assert isinstance(value, float)
        assert value == expected
        assert value == surface[index]


def test_negative_indices_resolve_from_end():
    surface, pointset = load_pointset()

    assert pointset[-1] == -0.0014
    assert pointset[-len(pointset)] == 0.0051
    assert pointset[-1] == surface[-1]


def test_positive_out_of_range_index_raises_indexerror():
    surface, pointset = load_pointset()

    with pytest.raises(
        IndexError, match=r"Invalid index, must be in \[0, 3871\), was: 3871\."
    ):
        pointset[len(surface)]


def test_too_negative_index_raises_indexerror():
    surface, pointset = load_pointset()

    with pytest.raises(
        IndexError, match=r"Invalid index, must be in \[0, 3871\), was: -3872\."
    ):
        pointset[-len(surface) - 1]


def test_pointset_reflects_surface_setitem_round_trip_and_equality_change():
    surface, pointset = load_pointset()
    original_copy = surface.copy()

    surface[0] = 123.456

    assert pointset[0] == 123.456
    assert surface.getPointset()[0] == 123.456
    assert pointset != original_copy.getPointset()
