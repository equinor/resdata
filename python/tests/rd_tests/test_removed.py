import time
import datetime

from resdata.util.test import TestAreaContext
from tests import ResdataTest
from resdata import ResDataType
from resdata.resfile import ResdataFile, ResdataKW, openFortIO, FortIO


class Removed_2_1_Test(ResdataTest):
    def test_rd_file_block(self):
        with TestAreaContext("name") as t:
            kw = ResdataKW("TEST", 3, ResDataType.RD_INT)
            with openFortIO("TEST", mode=FortIO.WRITE_MODE) as f:
                kw.fwrite(f)

            f = ResdataFile("TEST")
            with self.assertRaises(NotImplementedError):
                f.select_block("KW", 100)

            with self.assertRaises(NotImplementedError):
                f.select_global()

            with self.assertRaises(NotImplementedError):
                f.select_restart_section(index=None, report_step=None, sim_time=None)

            with self.assertRaises(NotImplementedError):
                f.select_restart_section()

            with self.assertRaises(NotImplementedError):
                ResdataFile.restart_block("TEST")
