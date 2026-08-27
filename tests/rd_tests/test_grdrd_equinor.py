#!/usr/bin/env python
import os

from resdata.grid import Grid
from resdata.resfile import Resdata3DKW, ResdataKW

from tests import ResdataTest, equinor_test


@equinor_test()
class GRDECLEquinorTest(ResdataTest):
    def setUp(self):
        self.src_file = self.createTestPath(
            "Equinor/ECLIPSE/Gurbat/include/example_permx.GRDECL"
        )
        self.file_list = []

    def addFile(self, filename):
        self.file_list.append(filename)

    def tearDown(self):
        for f in self.file_list:
            if os.path.exists(f):
                os.unlink(f)

    def test_Load(self):
        with open(self.src_file) as f:
            kw = ResdataKW.read_grdecl(f, "PERMX")
        self.assertTrue(kw)

        grid = Grid(self.createTestPath("Equinor/ECLIPSE/Gurbat/ECLIPSE"))
        with open(self.src_file) as f:
            kw = Resdata3DKW.read_grdecl(grid, f, "PERMX")
        self.assertTrue(isinstance(kw, Resdata3DKW))

    def test_reload(self):
        with open(self.src_file) as f:
            kw = ResdataKW.read_grdecl(f, "PERMX")
        tmp_file1 = "/tmp/permx1.grdecl"
        tmp_file2 = "/tmp/permx2.grdecl"
        self.addFile(tmp_file1)
        self.addFile(tmp_file2)

        with open(tmp_file1, "w") as fileH:
            kw.write_grdecl(fileH)

        with open(tmp_file1) as fileH:
            kw1 = ResdataKW.read_grdecl(fileH, "PERMX")

        with open(tmp_file2, "w") as fileH:
            kw1.write_grdecl(fileH)

        self.assertFilesAreEqual(tmp_file1, tmp_file2)

    def test_fseek(self):
        with open(self.src_file) as file:
            self.assertTrue(ResdataKW.fseek_grdecl(file, "PERMX"))
            self.assertFalse(ResdataKW.fseek_grdecl(file, "PERMY"))

        with open(self.src_file) as file:
            kw1 = ResdataKW.read_grdecl(file, "PERMX")
            self.assertFalse(ResdataKW.fseek_grdecl(file, "PERMX"))
            self.assertTrue(ResdataKW.fseek_grdecl(file, "PERMX", rewind=True))

    def test_fseek2(self):
        test_src = self.createTestPath("local/ECLIPSE/grdecl-test/test.grdecl")
        # Test kw at the the very start
        with open(test_src) as file:
            self.assertTrue(ResdataKW.fseek_grdecl(file, "PERMX"))

            # Test commented out kw:
            self.assertFalse(ResdataKW.fseek_grdecl(file, "PERMY"))
            self.assertFalse(ResdataKW.fseek_grdecl(file, "PERMZ"))

            # Test ignore not start of line:
            self.assertTrue(ResdataKW.fseek_grdecl(file, "MARKER"))
            self.assertFalse(ResdataKW.fseek_grdecl(file, "PERMXYZ"))

            # Test rewind
            self.assertFalse(ResdataKW.fseek_grdecl(file, "PERMX", rewind=False))
            self.assertTrue(ResdataKW.fseek_grdecl(file, "PERMX", rewind=True))

            # Test multiline comments + blanks
            self.assertTrue(ResdataKW.fseek_grdecl(file, "LASTKW"))

    def test_fseek_dos(self):
        test_src = self.createTestPath(
            "local/ECLIPSE/grdecl-test/test.grdecl_dos"
        )  # File formatted with \r\n line endings.
        # Test kw at the the very start
        with open(test_src) as file:
            self.assertTrue(ResdataKW.fseek_grdecl(file, "PERMX"))

            # Test commented out kw:
            self.assertFalse(ResdataKW.fseek_grdecl(file, "PERMY"))
            self.assertFalse(ResdataKW.fseek_grdecl(file, "PERMZ"))

            # Test ignore not start of line:
            self.assertTrue(ResdataKW.fseek_grdecl(file, "MARKER"))
            self.assertFalse(ResdataKW.fseek_grdecl(file, "PERMXYZ"))

            # Test rewind
            self.assertFalse(ResdataKW.fseek_grdecl(file, "PERMX", rewind=False))
            self.assertTrue(ResdataKW.fseek_grdecl(file, "PERMX", rewind=True))

            # Test multiline comments + blanks
            self.assertTrue(ResdataKW.fseek_grdecl(file, "LASTKW"))
