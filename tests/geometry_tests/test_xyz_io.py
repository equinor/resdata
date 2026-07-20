import pytest
from resdata.geometry import Polyline
from resdata.geometry.xyz_io import XYZIo


def test_that_reading_missing_xyz_file_raises_os_error():
    with pytest.raises(OSError, match="does not exist"):
        XYZIo.readXYZFile("does/not/exist.xyz")


def test_that_reading_missing_xy_file_raises_os_error():
    with pytest.raises(OSError, match="does not exist"):
        XYZIo.readXYFile("does/not/exist.xy")


def test_that_readxyzfile_reads_points_until_a_blank_line(tmp_path):
    path = tmp_path / "line.xyz"
    path.write_text("0 0 0\n1 1 1\n\n2 2 2\n")

    polyline = XYZIo.readXYZFile(str(path))

    assert polyline.getName() == "line.xyz"
    assert len(polyline) == 2
    assert polyline[0] == (0, 0, 0)
    assert polyline[1] == (1, 1, 1)


def test_that_readxyzfile_stops_at_the_999_sentinel(tmp_path):
    path = tmp_path / "sentinel.xyz"
    path.write_text("0 0 0\n999.000000 999.000000 999.000000\n2 2 2\n")

    polyline = XYZIo.readXYZFile(str(path))

    assert len(polyline) == 1
    assert polyline[0] == (0, 0, 0)


def test_that_a_polyline_saved_as_xy_can_be_read_back(tmp_path):
    original = Polyline(init_points=[(1.0, 0.0), (1.0, 1.0), (1.0, 2.0)])
    path = tmp_path / "poly.xy"

    XYZIo.saveXYFile(original, str(path))
    restored = XYZIo.readXYFile(str(path))

    assert original == restored
    assert restored.getName() == "poly.xy"
