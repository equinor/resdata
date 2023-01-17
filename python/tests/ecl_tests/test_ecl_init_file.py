from tests import EclTest, equinor_test
from ecl import EclFileFlagEnum
from ecl.eclfile import Ecl3DKW, EclKW, EclInitFile, EclFile, FortIO
from ecl.grid import EclGrid


@equinor_test()
class InitFileTest(EclTest):
    def setUp(self):
        self.grid_file = self.createTestPath("Equinor/ECLIPSE/Gurbat/ECLIPSE.EGRID")
        self.init_file = self.createTestPath("Equinor/ECLIPSE/Gurbat/ECLIPSE.INIT")

    def test_wrong_type(self):
        g = EclGrid(self.grid_file)
        with self.assertRaises(ValueError):
            f = EclInitFile(g, self.grid_file)

    def test_load(self):
        g = EclGrid(self.grid_file)
        f = EclInitFile(g, self.init_file)

        head = f["INTEHEAD"][0]
        self.assertTrue(isinstance(head, EclKW))

        porv = f["PORV"][0]
        self.assertTrue(isinstance(porv, Ecl3DKW))

        poro = f["PORO"][0]
        self.assertTrue(isinstance(poro, Ecl3DKW))
