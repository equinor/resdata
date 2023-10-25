#!/usr/bin/env python
try:
    from unittest2 import skipIf
except ImportError:
    from unittest import skipIf

from resdata import ResDataType
from resdata.resfile import ResdataKW
from resdata.grid import Grid
from tests import ResdataTest, equinor_test
from resdata.grid.faults import FaultBlock, FaultBlockLayer

from cwrap import open as copen


@equinor_test()
class FaultBlockTest(ResdataTest):
    def setUp(self):
        self.grid = Grid(self.createTestPath("Equinor/ECLIPSE/Mariner/MARINER.EGRID"))
        fileH = copen(self.createTestPath("Equinor/ECLIPSE/Mariner/faultblock.grdecl"))
        self.kw = ResdataKW.read_grdecl(fileH, "FAULTBLK", rd_type=ResDataType.RD_INT)

    def test_load(self):
        for k in range(self.grid.getNZ()):
            faultBlocks = FaultBlockLayer(self.grid, k)
            faultBlocks.scanKeyword(self.kw)
            for block in faultBlocks:
                centroid = block.getCentroid()
