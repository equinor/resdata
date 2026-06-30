import math
import os

import pytest
from resdata.geometry import Surface

SURFACE_FILE = "test-data/local/geometry/surface/valid_small_ascii.irap"
ROUNDTRIP_FILE = "tests/geometry_tests/surface_fault_roundtrip.irap"


def wrapped_surface(nx, ny, angle=0.0):
    return Surface(
        nx=nx,
        ny=ny,
        xinc=2.0,
        yinc=3.0,
        xstart=10.0,
        ystart=20.0,
        angle=angle,
    )


def test_constructor_wraps_large_nx_to_header_value():
    surface = wrapped_surface(2**32 + 2, 3)

    assert type(surface.getNX()) is int
    assert surface.getNX() == 2
    assert surface.getNY() == 3
    assert len(surface) == 6


def test_constructor_wraps_large_ny_to_header_value():
    surface = wrapped_surface(2, 2**32 + 3)

    assert type(surface.getNY()) is int
    assert surface.getNX() == 2
    assert surface.getNY() == 3
    assert len(surface) == 6


def test_constructor_wraps_both_dimensions_and_preserves_origin():
    surface = wrapped_surface(2**33 + 4, 2**32 + 5)

    assert surface.getNX() == 4
    assert surface.getNY() == 5
    assert len(surface) == 20
    assert surface.getXY(0) == (10.0, 20.0)


def test_wrapped_header_uses_exact_unrotated_corner_coordinates():
    surface = wrapped_surface(2**40 + 5, 2)

    assert surface.getNX() == 5
    assert surface.getNY() == 2
    assert surface.getXY(-1) == (18.0, 23.0)
    assert surface.getXYZ(idx=-1) == (18.0, 23.0, 0.0)


def test_wrapped_header_negative_index_matches_last_cell():
    surface = wrapped_surface(2**64 + 6, 2)

    assert surface.getNX() == 6
    assert surface[-1] == surface[len(surface) - 1]
    surface[-1] = 7.25
    assert surface[len(surface) - 1] == 7.25


def test_wrapped_header_out_of_range_getitem_raises_index_error():
    surface = wrapped_surface(2, 2**40 + 7)

    assert len(surface) == 14
    with pytest.raises(IndexError):
        surface[len(surface)]


def test_wrapped_header_out_of_range_setitem_raises_index_error():
    surface = wrapped_surface(2**32 + 8, 1)

    assert len(surface) == 8
    with pytest.raises(IndexError):
        surface[len(surface)] = 1.0


def test_wrapped_header_assign_copy_and_equality_round_trip():
    surface = wrapped_surface(2**32 + 3, 2**32 + 2)
    surface.assign(4.5)

    copy = surface.copy()

    assert copy == surface
    copy[0] = 1.25
    assert copy != surface
    assert surface[0] == 4.5


def test_wrapped_header_write_and_reload_preserves_equality():
    surface = wrapped_surface(2**32 + 3, 2)
    surface.assign(6.75)

    try:
        surface.write(ROUNDTRIP_FILE)
        reloaded = Surface(ROUNDTRIP_FILE)
        assert reloaded == surface
    finally:
        if os.path.exists(ROUNDTRIP_FILE):
            os.remove(ROUNDTRIP_FILE)


def test_pointset_from_loaded_surface_has_old_reference_parent_behavior():
    surface = Surface(SURFACE_FILE)
    pointset = surface.getPointset()

    assert len(pointset) == len(surface)
    assert pointset[0] == surface[0]
    assert pointset[-1] == surface[-1]
    assert pointset.isReference()
    assert pointset.parent() is None
