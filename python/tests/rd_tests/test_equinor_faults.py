#!/usr/bin/env python


from cwrap import open as copen
from resdata import ResDataType
from resdata.grid import Grid
from resdata.grid.faults import Fault, FaultCollection
from resdata.resfile import ResdataKW

from tests import ResdataTest, equinor_test


@equinor_test()
class EquinorFaultTest(ResdataTest):
    def loadGrid(self):
        grid_file = self.createTestPath("Equinor/ECLIPSE/Faults/grid.grdecl")
        fileH = copen(grid_file, "r")
        specgrid = ResdataKW.read_grdecl(
            fileH, "SPECGRID", rd_type=ResDataType.RD_INT, strict=False
        )
        zcorn = ResdataKW.read_grdecl(fileH, "ZCORN")
        coord = ResdataKW.read_grdecl(fileH, "COORD")
        actnum = ResdataKW.read_grdecl(fileH, "ACTNUM", rd_type=ResDataType.RD_INT)

        return Grid.create(specgrid, zcorn, coord, actnum)

    def test_load(self):
        grid = self.loadGrid()
        faults_file = self.createTestPath("Equinor/ECLIPSE/Faults/faults.grdecl")
        faults = FaultCollection(grid, faults_file)
        for fault in faults:
            for layer in fault:
                for fl in layer:
                    fl.verify()

    def test_splitLine2(self):
        grid = self.loadGrid()
        f = Fault(grid, "DF41_C")

        #                         179   180   181
        #           o     o     o     o     o     o     o     o     o     o     o     o     o     o
        #                             |
        #  78                         |
        #           o     o     o     o     o     o     o     o     o     o     o     o     o     o
        #                             |
        #  77                         |
        #           o     o     o     o     o     o     o     o     o     o     o     o     o     o
        #                             |
        #  76                         |
        #           o     o     o     o     o     o     o     o     o     o     o     o     o     o
        #                             |
        #  75                         |
        #           o     o     o     o     o     o     o     o     o     o     o     o     o     o
        #
        #  74
        #           o     o     o     o     o     o     o     o     o     o     o     o     o     o
        #
        #  73
        #           o     o     o     o-----o     o     o     o     o     o     o     o     o     o
        #                                   |
        #  72                               |
        #           o     o     o     o-----o     o     o     o     o     o     o     o     o     o
        #
        #  71
        #           o     o     o     o-----o     o     o     o     o     o     o     o     o     o
        #                                   |
        #  70                               |
        #           o     o     o     o     o     o     o     o     o     o     o     o     o     o
        #                                   |
        #  69                               |
        #           o     o     o     o     o     o     o     o     o     o     o     o     o     o
        #
        #  68
        #           o     o     o     o     o     o     o     o     o     o     o     o     o     o
        #
        #  67
        #           o     o     o     o     o     o     o     o     o     o     o     o     o     o
        #
        #  66
        #           o     o     o     o     o     o     o     o     o     o     o     o     o     o
        #                                   |
        #  65                               |
        #           o     o     o     o-----o     o     o     o     o     o     o     o     o     o

        f.addRecord(179, 179, 77, 78, 0, 42, "X")
        f.addRecord(179, 179, 75, 76, 0, 41, "X")
        f.addRecord(180, 180, 72, 72, 0, 41, "X")
        f.addRecord(180, 180, 72, 72, 0, 41, "Y")
        f.addRecord(180, 180, 72, 72, 0, 41, "Y-")

        f.addRecord(180, 180, 70, 70, 0, 42, "Y")
        f.addRecord(180, 180, 69, 70, 0, 42, "X")
        f.addRecord(180, 180, 65, 65, 0, 42, "X")
        f.addRecord(180, 180, 65, 65, 0, 42, "Y-")

        ij_polyline = f.getIJPolyline(19)
        ij_list = [
            (180, 79),
            (180, 77),
            (180, 75),
            (180, 73),
            (181, 73),
            (181, 72),
            (180, 72),
            (180, 71),
            (181, 71),
            (181, 69),
            (181, 66),
            (181, 65),
            (180, 65),
        ]

        self.assertEqual(ij_polyline, ij_list)
