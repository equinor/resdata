"""
Simple package for working with 2D geometry.

"""

import resdata  # noqa
from cwrap import Prototype  # noqa: F401

from .cpolyline import CPolyline
from .cpolyline_collection import CPolylineCollection
from .geo_pointset import GeoPointset
from .geo_region import GeoRegion
from .geometry_tools import GeometryTools
from .polyline import Polyline
from .surface import Surface
from .xyz_io import XYZIo

__all__ = [
    "CPolyline",
    "CPolylineCollection",
    "GeoPointset",
    "GeoRegion",
    "GeometryTools",
    "Polyline",
    "Surface",
    "XYZIo",
]
