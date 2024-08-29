from tests import ResdataTest, equinor_test
from resdata.resfile import Resdata3DKW, ResdataKW, ResdataInitFile
from resdata.grid import Grid


@equinor_test()
class InitFileTest(ResdataTest):
    def setUp(self):
        self.grid_file = self.createTestPath("Equinor/ECLIPSE/Gurbat/ECLIPSE.EGRID")
        self.init_file = self.createTestPath("Equinor/ECLIPSE/Gurbat/ECLIPSE.INIT")

    def test_wrong_type(self):
        g = Grid(self.grid_file)
        with self.assertRaises(ValueError):
            f = ResdataInitFile(g, self.grid_file)

    def test_load(self):
        g = Grid(self.grid_file)
        f = ResdataInitFile(g, self.init_file)

        head = f["INTEHEAD"][0]
        self.assertTrue(isinstance(head, ResdataKW))

        porv = f["PORV"][0]
        self.assertTrue(isinstance(porv, Resdata3DKW))

        poro = f["PORO"][0]
        self.assertTrue(isinstance(poro, Resdata3DKW))
