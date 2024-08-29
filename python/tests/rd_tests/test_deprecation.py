#!/usr/bin/env python
import warnings

from resdata import ResDataType
from resdata.grid import GridGenerator, ResdataRegion
from resdata.resfile import FortIO, ResdataFile, ResdataKW, openFortIO
from resdata.util.test import TestAreaContext
from tests import ResdataTest

# The class Deprecation_1_9_Test contains methods which will be marked
# as deprecated in the 1.9.x versions.

warnings.simplefilter("error", DeprecationWarning)


class Deprecation_2_1_Test(ResdataTest):
    pass


class Deprecation_2_0_Test(ResdataTest):
    def test_ResdataFile_name_property(self):
        with TestAreaContext("name") as t:
            kw = ResdataKW("TEST", 3, ResDataType.RD_INT)
            with openFortIO("TEST", mode=FortIO.WRITE_MODE) as f:
                kw.fwrite(f)

            f = ResdataFile("TEST")


class Deprecation_1_9_Test(ResdataTest):
    def test_ResdataRegion_properties(self):
        grid = GridGenerator.createRectangular((10, 10, 10), (1, 1, 1))
        region = ResdataRegion(grid, False)
