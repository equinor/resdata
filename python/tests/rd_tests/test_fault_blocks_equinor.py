#!/usr/bin/env python

from cwrap import open as copen
from resdata import ResDataType
from resdata.grid import Grid
from resdata.grid.faults import FaultBlockLayer
from resdata.resfile import ResdataKW

from tests import ResdataTest, equinor_test


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
                _centroid = block.getCentroid()
