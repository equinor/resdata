"""
Simple package for working with 2D geometry.

"""

from cwrap import Prototype as Prototype

import resdata as resdata

from .cpolyline import CPolyline as CPolyline
from .cpolyline_collection import CPolylineCollection as CPolylineCollection
from .geo_pointset import GeoPointset as GeoPointset
from .geo_region import GeoRegion as GeoRegion
from .geometry_tools import GeometryTools as GeometryTools
from .polyline import Polyline as Polyline
from .surface import Surface as Surface
from .xyz_io import XYZIo as XYZIo
