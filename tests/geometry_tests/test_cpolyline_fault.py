import pytest
from resdata.geometry import CPolyline


def test_constructor_rejects_integer_name_with_ctypes_error():
    with pytest.raises(TypeError, match="CStringHelper"):
        CPolyline(name=123)


def test_constructor_rejects_object_name_with_ctypes_error():
    with pytest.raises(TypeError, match="CStringHelper"):
        CPolyline(name=object())


def test_create_from_xyz_rejects_integer_name_with_ctypes_error():
    with pytest.raises(TypeError, match="CStringHelper"):
        CPolyline.createFromXYZFile("test-data/local/geometry/pol8.xyz", name=123)


def test_add_point_rejects_string_x_with_ctypes_double_error():
    polyline = CPolyline()
    with pytest.raises(TypeError, match=r"Argument 1:.*ctypes\\.c_double"):
        polyline.addPoint("bad", 1.0)


def test_add_point_rejects_string_y_with_ctypes_double_error():
    polyline = CPolyline()
    with pytest.raises(TypeError, match=r"Argument 2:.*ctypes\\.c_double"):
        polyline.addPoint(1.0, "bad")


def test_add_point_front_rejects_string_x_with_ctypes_double_error():
    polyline = CPolyline()
    with pytest.raises(TypeError, match=r"Argument 1:.*ctypes\\.c_double"):
        polyline.addPoint("bad", 1.0, front=True)


def test_segment_intersects_rejects_string_first_x_with_ctypes_double_error():
    polyline = CPolyline(init_points=[(0.0, 0.0), (1.0, 1.0)])
    with pytest.raises(TypeError, match=r"Argument 1:.*ctypes\\.c_double"):
        polyline.segmentIntersects(("bad", 0.0), (1.0, 1.0))


def test_segment_intersects_rejects_string_first_y_with_ctypes_double_error():
    polyline = CPolyline(init_points=[(0.0, 0.0), (1.0, 1.0)])
    with pytest.raises(TypeError, match=r"Argument 2:.*ctypes\\.c_double"):
        polyline.segmentIntersects((0.0, "bad"), (1.0, 1.0))


def test_segment_intersects_rejects_string_second_x_with_ctypes_double_error():
    polyline = CPolyline(init_points=[(0.0, 0.0), (1.0, 1.0)])
    with pytest.raises(TypeError, match=r"Argument 3:.*ctypes\\.c_double"):
        polyline.segmentIntersects((0.0, 0.0), ("bad", 1.0))


def test_segment_intersects_rejects_string_second_y_with_ctypes_double_error():
    polyline = CPolyline(init_points=[(0.0, 0.0), (1.0, 1.0)])
    with pytest.raises(TypeError, match=r"Argument 4:.*ctypes\\.c_double"):
        polyline.segmentIntersects((0.0, 0.0), (1.0, "bad"))
