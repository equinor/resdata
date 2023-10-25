"""
Simple package for working with 2D geometry.

"""
import resdata
from cwrap import Prototype

from .geo_pointset import GeoPointset
from .geo_region import GeoRegion
from .cpolyline import CPolyline
from .cpolyline_collection import CPolylineCollection
from .polyline import Polyline
from .xyz_io import XYZIo
from .geometry_tools import GeometryTools
from .surface import Surface
