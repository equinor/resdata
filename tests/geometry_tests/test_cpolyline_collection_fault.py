import pytest
from resdata.geometry import CPolyline, CPolylineCollection


def populated_collection():
    collection = CPolylineCollection()
    collection.createPolyline(name="created")
    collection.addPolyline(CPolyline(name="added", init_points=[(1, 2), (3, 4)]))
    collection.addPolyline([(5, 6), (7, 8)], name="from_points")
    return collection


def test_name_lookup_created_polyline_has_no_parent_reference():
    collection = populated_collection()

    assert collection["created"].parent() is None


def test_name_lookup_added_polyline_has_no_parent_reference():
    collection = populated_collection()

    assert collection["added"].parent() is None


def test_name_lookup_points_polyline_has_no_parent_reference():
    collection = populated_collection()

    assert collection["from_points"].parent() is None


def test_name_lookup_unicode_polyline_has_no_parent_reference():
    collection = CPolylineCollection()
    collection.createPolyline(name="å-name")

    assert collection["å-name"].parent() is None


def test_name_lookup_empty_name_polyline_has_no_parent_reference():
    collection = CPolylineCollection()
    collection.createPolyline(name="")

    assert collection[""].parent() is None


def test_duplicate_empty_name_raises_attribute_error():
    collection = CPolylineCollection()
    collection.createPolyline(name="")

    with pytest.raises(AttributeError, match="setParent"):
        collection.createPolyline(name="")


def test_contains_int_name_reports_ctypes_string_conversion_error():
    collection = CPolylineCollection()

    with pytest.raises(TypeError, match="CStringHelper"):
        1 in collection


def test_contains_object_name_reports_ctypes_string_conversion_error():
    collection = CPolylineCollection()

    with pytest.raises(TypeError, match="CStringHelper"):
        object() in collection


def test_create_polyline_int_name_reports_ctypes_string_conversion_error():
    collection = CPolylineCollection()

    with pytest.raises(TypeError, match="CStringHelper"):
        collection.createPolyline(name=1)


def test_add_points_int_name_reports_ctypes_string_conversion_error():
    collection = CPolylineCollection()

    with pytest.raises(TypeError, match="CStringHelper"):
        collection.addPolyline([(1, 2), (3, 4)], name=1)
