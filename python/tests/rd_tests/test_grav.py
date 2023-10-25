import time
from resdata import ResDataType
from resdata.resfile import ResdataKW, ResdataFile, openFortIO, FortIO
from resdata.grid import Grid
from resdata.gravimetry import ResdataGrav
from resdata.util.test import TestAreaContext
from tests import ResdataTest


class ResdataGravTest(ResdataTest):
    def setUp(self):
        self.grid = Grid.createRectangular((10, 10, 10), (1, 1, 1))

    def test_create(self):
        # The init file created here only contains a PORO field. More
        # properties must be added to this before it can be used for
        # any usefull gravity calculations.
        poro = ResdataKW("PORO", self.grid.getGlobalSize(), ResDataType.RD_FLOAT)
        with TestAreaContext("grav_init"):
            with openFortIO("TEST.INIT", mode=FortIO.WRITE_MODE) as f:
                poro.fwrite(f)
            self.init = ResdataFile("TEST.INIT")

            grav = ResdataGrav(self.grid, self.init)
