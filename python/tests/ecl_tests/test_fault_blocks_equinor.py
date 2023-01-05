#!/usr/bin/env python
try:
    from unittest2 import skipIf
except ImportError:
    from unittest import skipIf

from ecl import EclDataType
from ecl.eclfile import EclKW
from ecl.grid import EclGrid
from tests import EclTest, equinor_test
from ecl.grid.faults import FaultBlock, FaultBlockLayer

from cwrap import open as copen


@equinor_test()
class FaultBlockTest(EclTest):
    def setUp(self):
        self.grid = EclGrid(
            self.createTestPath("Equinor/ECLIPSE/Mariner/MARINER.EGRID")
        )
        fileH = copen(self.createTestPath("Equinor/ECLIPSE/Mariner/faultblock.grdecl"))
        self.kw = EclKW.read_grdecl(fileH, "FAULTBLK", ecl_type=EclDataType.ECL_INT)

    def test_load(self):
        for k in range(self.grid.getNZ()):
            faultBlocks = FaultBlockLayer(self.grid, k)
            faultBlocks.scanKeyword(self.kw)
            for block in faultBlocks:
                centroid = block.getCentroid()
