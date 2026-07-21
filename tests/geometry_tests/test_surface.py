import random

import hypothesis.strategies as st
import pytest
from hypothesis import given
from resdata.geometry import Surface

from tests import ResdataTest


class SurfaceTest(ResdataTest):
    def setUp(self):
        self.surface_valid = self.createTestPath(
            "local/geometry/surface/valid_ascii.irap"
        )
        self.surface_short = self.createTestPath(
            "local/geometry/surface/short_ascii.irap"
        )
        self.surface_long = self.createTestPath(
            "local/geometry/surface/long_ascii.irap"
        )
        self.surface_valid2 = self.createTestPath(
            "local/geometry/surface/valid2_ascii.irap"
        )
        self.surface_small = self.createTestPath(
            "local/geometry/surface/valid_small_ascii.irap"
        )

    def test_xyz(self):
        s = Surface(self.surface_valid2)
        self.assertEqual(s.getXYZ(i=5, j=13), s.getXYZ(idx=642))
        x, y, z = s.getXYZ(i=5, j=13)
        self.assertFloatEqual(464041.44804, x)
        self.assertFloatEqual(7336966.309535, y)
        self.assertFloatEqual(0.0051, z)
        self.assertAlmostEqualList(s.getXYZ(i=6, j=13), s.getXYZ(idx=643))
        self.assertFloatEqual(-0.0006, s.getXYZ(i=6, j=13)[2])  # z value

        nx = s.getNX()
        ny = s.getNY()
        self.assertEqual(s.getXYZ(i=-1, j=-1), s.getXYZ(i=nx - 1, j=ny - 1))

    def test_repr(self):
        s = Surface(self.surface_valid)
        r = repr(s)
        self.assertIn("nx=49", r)
        self.assertIn("ny=79", r)

    def test_create_new(self):
        with self.assertRaises(ValueError):
            s = Surface(None, 1, 1, 1)
        with self.assertRaises(IOError):
            s = Surface(50, 1, 1, 1)

        # values copied from irap surface_small
        ny, nx = 20, 30
        xinc, yinc = 50.0, 50.0
        xstart, ystart = 463325.5625, 7336963.5
        angle = -65.0
        s_args = (None, nx, ny, xinc, yinc, xstart, ystart, angle)
        s = Surface(*s_args)
        self.assertEqual(ny * nx, len(s))
        self.assertEqual(nx, s.getNX())
        self.assertEqual(ny, s.getNY())
        small = Surface(self.surface_small)
        self.assertTrue(small.headerEqual(s))
        valid = Surface(self.surface_valid)
        self.assertFalse(valid.headerEqual(s))

        self.assertNotEqual(s, small)
        idx = 0
        for i in range(nx):
            for j in range(ny):
                s[idx] = small[idx]
                idx += 1
        self.assertEqual(s, small)

    def test_create(self):
        with self.assertRaises(IOError):
            s = Surface("File/does/not/exist")

        with self.assertRaises(ValueError):
            s = Surface(self.surface_short)

        with self.assertRaises(ValueError):
            s = Surface(self.surface_long)

        s = Surface(self.surface_valid)

        self.assertEqual(s.getNX(), 49)
        self.assertEqual(s.getNY(), 79)
        self.assertEqual(len(s), 49 * 79)

        with self.assertRaises(IndexError):
            v = s[49 * 79]

        with self.assertRaises(TypeError):
            v = s["KEY"]

        self.assertEqual(s[0], 0.0051)
        self.assertEqual(s[-1], -0.0014)

        with self.assertRaises(IndexError):
            s[49 * 79] = 787

        s[0] = 10
        self.assertEqual(s[0], 10)

        s[-1] = 77
        self.assertEqual(s[len(s) - 1], 77)

    def test_write(self):
        tmpdir = self.tmp_path_factory.mktemp("surface_write", numbered=True)
        with self.monkeypatch.context() as mp:
            mp.chdir(tmpdir)
            s0 = Surface(self.surface_valid)
            s0.write("new_surface.irap")

            s1 = Surface("new_surface.irap")
            self.assertTrue(s1 == s0)

            s0[0] = 99
            self.assertFalse(s1 == s0)

    def test_copy(self):
        tmpdir = self.tmp_path_factory.mktemp("surface_copy", numbered=True)
        with self.monkeypatch.context() as mp:
            mp.chdir(tmpdir)
            s0 = Surface(self.surface_valid)
            s1 = s0.copy()

            self.assertTrue(s1 == s0)
            s1[0] = 99
            self.assertFalse(s1 == s0)
            del s0
            self.assertEqual(s1[0], 99)

            s2 = s1.copy(copy_data=False)
            self.assertEqual(s2[0], 0.0)
            self.assertEqual(s2[10], 0.0)
            self.assertEqual(s2[100], 0.0)

    def test_header_equal(self):
        s0 = Surface(self.surface_valid)
        s1 = Surface(self.surface_valid2)
        s2 = s0.copy()

        self.assertTrue(s0.headerEqual(s0))
        self.assertFalse(s0.headerEqual(s1))
        self.assertTrue(s0.headerEqual(s2))

    def test_ops(self):
        s0 = Surface(self.surface_valid)
        s0.assign(1.0)
        for v in s0:
            self.assertEqual(v, 1.0)

        s0 += 1
        for v in s0:
            self.assertEqual(v, 2.0)

        s0 *= 2
        for v in s0:
            self.assertEqual(v, 4.0)

        s1 = s0 + 4
        for v in s1:
            self.assertEqual(v, 8.0)

        s2 = Surface(self.surface_valid2)
        with self.assertRaises(ValueError):
            s3 = s1 + s2

        s4 = s1 + s0
        for v in s4:
            self.assertEqual(v, 12.0)

        s5 = s4 / 12
        for v in s5:
            self.assertEqual(v, 1.0)

    def test_ops2(self):
        s0 = Surface(self.surface_small)
        surface_list = []
        for i in range(10):
            s = s0.copy()
            for j in range(len(s)):
                s[j] = random.random()
            surface_list.append(s)

        mean = s0.copy(copy_data=False)
        for s in surface_list:
            mean += s
        mean /= len(surface_list)

        std = s0.copy(copy_data=False)
        for s in surface_list:
            std += (s - mean) * (s - mean)
        std /= len(surface_list) - 1

    def test_sqrt(self):
        s0 = Surface(self.surface_small)
        s0.assign(4)
        self.assertEqual(20 * 30, len(s0))
        s_sqrt = s0.sqrt()
        for i in range(len(s0)):
            self.assertEqual(s0[i], 4)
            self.assertEqual(s_sqrt[i], 2)
        s0.inplaceSqrt()
        self.assertTrue(s0 == s_sqrt)

    def test_xy(self):
        ny, nx = 20, 30
        xinc, yinc = 50.0, 50.0
        xstart, ystart = 463325.5625, 7336963.5
        angle = 0
        s_args = (None, nx, ny, xinc, yinc, xstart, ystart, angle)
        s = Surface(*s_args)

        xy = s.getXY(0)
        self.assertEqual((xstart, ystart), xy)

        xy = s.getXY(1)
        self.assertEqual((xstart + xinc, ystart), xy)

        xy = s.getXY(nx)
        self.assertEqual((xstart, ystart + yinc), xy)

        xy = s.getXY(-1)
        self.assertEqual((xstart + xinc * (nx - 1), ystart + yinc * (ny - 1)), xy)


def _make_surface(nx=3, ny=2):
    return Surface(nx=nx, ny=ny, xinc=1.0, yinc=1.0, xstart=0.0, ystart=0.0, angle=0.0)


def test_that_a_surface_is_not_equal_to_a_non_surface():
    surface = _make_surface()

    assert surface != 5


def test_that_subtracting_incompatible_surfaces_raises_value_error():
    surface = _make_surface(3, 2)
    other = _make_surface(2, 2)

    with pytest.raises(ValueError, match="incompatible surfaces"):
        surface -= other


def test_that_multiplying_incompatible_surfaces_raises_value_error():
    surface = _make_surface(3, 2)
    other = _make_surface(2, 2)

    with pytest.raises(ValueError, match="incompatible surfaces"):
        surface *= other


def test_that_subtracting_a_scalar_shifts_every_value():
    surface = _make_surface()
    surface.assign(5.0)

    surface -= 2.0

    assert all(surface[i] == pytest.approx(3.0) for i in range(len(surface)))


def test_that_idiv_scales_every_value_by_the_inverse():
    surface = _make_surface()
    surface.assign(8.0)

    result = surface.__idiv__(2.0)

    assert all(result[i] == pytest.approx(4.0) for i in range(len(result)))


def test_that_div_returns_a_scaled_copy_without_mutating_the_original():
    surface = _make_surface()
    surface.assign(8.0)

    copy = surface.__div__(2.0)

    assert all(copy[i] == pytest.approx(4.0) for i in range(len(copy)))
    assert all(surface[i] == pytest.approx(8.0) for i in range(len(surface)))


def test_that_setting_a_value_with_a_non_int_index_raises_type_error():
    surface = _make_surface()

    with pytest.raises(TypeError, match="must be integer"):
        surface["x"] = 1.0


def test_that_getxy_out_of_range_raises_index_error():
    surface = _make_surface()

    with pytest.raises(IndexError):
        surface.getXY(len(surface) + 10)


def test_that_getxy_with_a_non_int_index_raises_type_error():
    surface = _make_surface()

    with pytest.raises(TypeError, match="must be integer"):
        surface.getXY("x")


def test_that_getxyz_without_idx_or_ij_raises_value_error():
    surface = _make_surface()

    with pytest.raises(ValueError, match="i and j must be ints"):
        surface.getXYZ()


def test_that_getxyz_with_both_idx_and_ij_raises_value_error():
    surface = _make_surface()

    with pytest.raises(ValueError, match="i and j must be None"):
        surface.getXYZ(idx=0, i=1)


def test_that_getxyz_with_out_of_range_ij_raises_index_error():
    surface = _make_surface(3, 2)

    with pytest.raises(IndexError, match="Index error"):
        surface.getXYZ(i=100, j=0)


def test_that_getxyz_by_index_matches_getxyz_by_ij():
    surface = _make_surface(3, 2)
    surface.assign(1.0)

    idx = 1 * surface.getNX() + 2  # j=1, i=2

    assert surface.getXYZ(idx=idx) == surface.getXYZ(i=2, j=1)


def test_that_loading_a_surface_with_a_malformed_header_raises_value_error(tmp_path):
    path = tmp_path / "malformed_header.irap"
    path.write_text("this is not a surface\n")

    with pytest.raises(ValueError, match="reading irap header failed"):
        Surface(str(path))


def test_that_loading_a_surface_with_a_truncated_header_raises_value_error(tmp_path):
    path = tmp_path / "truncated_header.irap"
    path.write_text("-996 3 50 50 0 100 0 100 4 0\n0.0 0.0\n")

    with pytest.raises(ValueError, match="reading irap header failed"):
        Surface(str(path))


def test_that_loading_a_surface_with_a_negative_size_raises_value_error(tmp_path):
    path = tmp_path / "negative_size.irap"
    path.write_text("-996 -3 50 50 0 100 0 100 -4 0\n0.0 0.0 0 0 0 0 0 0 0\n")

    with pytest.raises(ValueError, match="surface size was negative"):
        Surface(str(path))


# The irap ascii format stores values with four decimals, so we quantize every
# input to four decimals to make the write/read round trip lossless.
def _quantized_floats(min_value, max_value):
    return st.floats(
        min_value=min_value,
        max_value=max_value,
        allow_nan=False,
        allow_infinity=False,
    ).map(lambda value: round(value, 4))


@st.composite
def surfaces(draw):
    nx = draw(st.integers(min_value=1, max_value=8))
    ny = draw(st.integers(min_value=1, max_value=8))
    surface = draw(
        st.builds(
            Surface,
            nx=st.just(nx),
            ny=st.just(ny),
            xinc=_quantized_floats(0.1, 1000.0),
            yinc=_quantized_floats(0.1, 1000.0),
            xstart=_quantized_floats(-10000.0, 10000.0),
            ystart=_quantized_floats(-10000.0, 10000.0),
            angle=_quantized_floats(-180.0, 180.0),
        )
    )
    zvalues = draw(
        st.lists(
            _quantized_floats(-10000.0, 10000.0),
            min_size=nx * ny,
            max_size=nx * ny,
        )
    )
    for index, zvalue in enumerate(zvalues):
        surface[index] = zvalue
    return surface


@given(surface=surfaces())
def test_that_writing_and_reading_a_surface_returns_an_equal_surface(
    surface, tmp_path_factory
):
    path = tmp_path_factory.mktemp("surface_roundtrip") / "surface.irap"

    surface.write(str(path))

    assert Surface(filename=str(path)) == surface


def test_that_writing_a_surface_creates_missing_parent_directories(tmp_path):
    surface = _make_surface()
    path = tmp_path / "missing" / "nested" / "surface.irap"

    assert not path.parent.exists()

    surface.write(str(path))

    assert path.exists()
    assert Surface(filename=str(path)) == surface
