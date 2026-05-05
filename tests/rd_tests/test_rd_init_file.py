import os
from pathlib import Path

import pytest

from tests import ResdataTest, equinor_test, source_root
from resdata import FileMode
from resdata.resfile import Resdata3DKW, ResdataKW, ResdataInitFile, ResdataFile, FortIO
from resdata.grid import Grid


def test_load_local_init_file():
    base = Path(source_root()) / "test-data" / "local" / "ECLIPSE" / "rft"
    grid = Grid(str(base / "SPE1.EGRID"))
    init = ResdataInitFile(grid, str(base / "SPE1.INIT"))

    porv = init["PORV"][0]
    assert isinstance(porv, Resdata3DKW)


def test_init_file_wrong_type_raises(tmp_path):
    base = Path(source_root()) / "test-data" / "local" / "ECLIPSE" / "rft"
    grid = Grid(str(base / "SPE1.EGRID"))
    with pytest.raises(ValueError, match="does not correspond to an init file"):
        ResdataInitFile(grid, str(base / "SPE1.EGRID"))


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
