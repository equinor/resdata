#!/usr/bin/env python
import os
import cwrap
from random import randint
from resdata import ResDataType
from resdata.resfile import FortIO, ResdataKW, openFortIO, ResdataFile
from resdata.util.test import TestAreaContext
from tests import ResdataTest


class FortIOTest(ResdataTest):
    def test_open_write(self):
        with TestAreaContext("python/fortio/write"):
            f = FortIO("newfile", FortIO.WRITE_MODE)
            self.assertTrue(os.path.exists("newfile"))

    def test_noex(self):
        with self.assertRaises(IOError):
            f = FortIO("odes_not_exist", FortIO.READ_MODE)

    def test_kw(self):
        kw1 = ResdataKW("KW1", 2, ResDataType.RD_INT)
        kw2 = ResdataKW("KW2", 2, ResDataType.RD_INT)

        kw1[0] = 99
        kw1[1] = 77
        kw2[0] = 113
        kw2[1] = 335

        with TestAreaContext("python/fortio/write-kw"):
            f = FortIO("test", FortIO.WRITE_MODE, fmt_file=False)
            kw1.fwrite(f)

            f = FortIO("test", FortIO.APPEND_MODE)
            kw2.fwrite(f)

            f = FortIO("test", fmt_file=False)
            k1 = ResdataKW.fread(f)
            k2 = ResdataKW.fread(f)

            self.assertTrue(k1.equal(kw1))
            self.assertTrue(k2.equal(kw2))

    def test_truncate(self):
        kw1 = ResdataKW("KW1", 2, ResDataType.RD_INT)
        kw2 = ResdataKW("KW2", 2, ResDataType.RD_INT)

        kw1[0] = 99
        kw1[1] = 77
        kw2[0] = 113
        kw2[1] = 335

        with TestAreaContext("python/fortio/ftruncate") as t:
            with openFortIO("file", mode=FortIO.WRITE_MODE) as f:
                kw1.fwrite(f)
                pos1 = f.getPosition()
                kw2.fwrite(f)

            # Truncate file in read mode; should fail hard.
            with openFortIO("file") as f:
                with self.assertRaises(IOError):
                    f.truncate()

            with openFortIO("file", mode=FortIO.READ_AND_WRITE_MODE) as f:
                f.seek(pos1)
                f.truncate()

            f = ResdataFile("file")
            self.assertEqual(len(f), 1)
            kw1_ = f[0]
            self.assertEqual(kw1, kw1_)

    def test_fortio_creation(self):
        with TestAreaContext("python/fortio/create"):
            w = FortIO("test", FortIO.WRITE_MODE)
            rw = FortIO("test", FortIO.READ_AND_WRITE_MODE)
            r = FortIO("test", FortIO.READ_MODE)
            a = FortIO("test", FortIO.APPEND_MODE)

            w.close()
            w.close()  # should not fail

    def test_context(self):
        with TestAreaContext("python/fortio/context") as t:
            kw1 = ResdataKW("KW", 2456, ResDataType.RD_FLOAT)
            for i in range(len(kw1)):
                kw1[i] = randint(0, 1000)

            with openFortIO("file", mode=FortIO.WRITE_MODE) as f:
                kw1.fwrite(f)
                self.assertEqual(f.filename(), "file")

            with openFortIO("file") as f:
                kw2 = ResdataKW.fread(f)

            self.assertTrue(kw1 == kw2)

    def test_is_fortran_file(self):
        with TestAreaContext("python/fortio/guess"):
            kw1 = ResdataKW("KW", 12345, ResDataType.RD_FLOAT)
            with openFortIO("fortran_file", mode=FortIO.WRITE_MODE) as f:
                kw1.fwrite(f)

            with cwrap.open("text_file", "w") as f:
                kw1.write_grdecl(f)

            self.assertTrue(FortIO.isFortranFile("fortran_file"))
            self.assertFalse(FortIO.isFortranFile("text_file"))
