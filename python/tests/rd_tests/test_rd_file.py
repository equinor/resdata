import shutil
import datetime
import os.path
import gc

from resdata import FileMode, ResDataType, FileType
from resdata.resfile import ResdataFile, FortIO, ResdataKW, openFortIO, openResdataFile
from resdata.util.util import CWDContext
from resdata.util.test import TestAreaContext
from resdata.grid import Grid
from tests import ResdataTest
from tests.rd_tests.create_restart import create_restart


def createFile(name, kw_list):
    with openFortIO(name, mode=FortIO.WRITE_MODE) as f:
        for kw in kw_list:
            kw.fwrite(f)


def loadKeywords(name):
    kw_list = []
    f = ResdataFile(name)
    for kw in f:
        kw_list.append(kw)

    return kw_list


class ResdataFileTest(ResdataTest):
    def assertFileType(self, filename, expected):
        file_type, step, fmt_file = ResdataFile.getFileType(filename)
        self.assertEqual(file_type, expected[0])
        self.assertEqual(fmt_file, expected[1])
        self.assertEqual(step, expected[2])

    def test_file_type(self):
        self.assertFileType("ECLIPSE.UNRST", (FileType.UNIFIED_RESTART, False, None))
        self.assertFileType("ECLIPSE.X0030", (FileType.RESTART, False, 30))
        self.assertFileType("ECLIPSE.DATA", (FileType.DATA, None, None))
        self.assertFileType("ECLIPSE.FINIT", (FileType.INIT, True, None))
        self.assertFileType("ECLIPSE.A0010", (FileType.SUMMARY, True, 10))
        self.assertFileType("ECLIPSE.EGRID", (FileType.EGRID, False, None))

    def test_IOError(self):
        with self.assertRaises(IOError):
            ResdataFile("No/Does/not/exist")

    def test_context(self):
        with TestAreaContext("python/rd_file/context"):
            kw1 = ResdataKW("KW1", 100, ResDataType.RD_INT)
            kw2 = ResdataKW("KW2", 100, ResDataType.RD_INT)
            with openFortIO("TEST", mode=FortIO.WRITE_MODE) as f:
                kw1.fwrite(f)
                kw2.fwrite(f)

            self.assertFalse(ResdataFile.contains_report_step("TEST", 99))
            self.assertFalse(
                ResdataFile.contains_sim_time("TEST", datetime.datetime(2024, 4, 4))
            )
            with openResdataFile("TEST") as rd_file:
                assert rd_file.iget_named_kw("KW1", 0).name == "KW1"
                self.assertEqual(len(rd_file), 2)
                self.assertTrue(rd_file.has_kw("KW1"))
                self.assertTrue(rd_file.has_kw("KW2"))
                self.assertFalse(rd_file.has_report_step(99))
                self.assertFalse(rd_file.has_sim_time(datetime.datetime(2024, 4, 4)))
                self.assertEqual(rd_file[1], rd_file[-1])

                self.assertEqual(rd_file.iget_kw(0).name, "KW1")

    def test_rd_index(self):
        with TestAreaContext("python/rd_file/context"):
            kw1 = ResdataKW("KW1", 100, ResDataType.RD_INT)
            kw2 = ResdataKW("KW2", 100, ResDataType.RD_FLOAT)
            kw3 = ResdataKW("KW3", 100, ResDataType.RD_CHAR)
            kw4 = ResdataKW("KW4", 100, ResDataType.RD_STRING(23))
            with openFortIO("TEST", mode=FortIO.WRITE_MODE) as f:
                kw1.fwrite(f)
                kw2.fwrite(f)
                kw3.fwrite(f)
                kw4.fwrite(f)

            rd_file = ResdataFile("TEST")
            rd_file.write_index("INDEX_FILE")
            rd_file.close()

            rd_file_index = ResdataFile("TEST", 0, "INDEX_FILE")
            for kw in ["KW1", "KW2", "KW3", "KW4"]:
                self.assertIn(kw, rd_file_index)

            with self.assertRaises(IOError):
                rd_file.write_index("does-not-exist/INDEX")

            with self.assertRaises(IOError):
                rd_file_index = ResdataFile("TEST", 0, "index_does_not_exist")

            os.mkdir("path")
            shutil.copyfile("TEST", "path/TEST")
            rd_file = ResdataFile("path/TEST")
            rd_file.write_index("path/index")

            with CWDContext("path"):
                rd_file = ResdataFile("TEST", 0, "index")

    def test_save_kw(self):
        with TestAreaContext("python/rd_file/save_kw"):
            data = range(1000)
            kw = ResdataKW("MY_KEY", len(data), ResDataType.RD_INT)
            for index, val in enumerate(data):
                kw[index] = val

            clean_dump = "my_clean_file"
            fortio = FortIO(clean_dump, FortIO.WRITE_MODE)
            kw.fwrite(fortio)
            fortio.close()

            test_file = "my_dump_file"
            fortio = FortIO(test_file, FortIO.WRITE_MODE)
            kw.fwrite(fortio)
            fortio.close()

            self.assertFilesAreEqual(clean_dump, test_file)

            rd_file = ResdataFile(test_file, flags=FileMode.WRITABLE)
            loaded_kw = rd_file["MY_KEY"][0]
            self.assertTrue(kw.equal(loaded_kw))

            rd_file.save_kw(loaded_kw)
            rd_file.close()

            self.assertFilesAreEqual(clean_dump, test_file)

            rd_file = ResdataFile(test_file)
            loaded_kw = rd_file["MY_KEY"][0]
            self.assertTrue(kw.equal(loaded_kw))

    def test_gc(self):
        kw1 = ResdataKW("KW1", 100, ResDataType.RD_INT)
        kw2 = ResdataKW("KW2", 100, ResDataType.RD_INT)
        kw3 = ResdataKW("KW3", 100, ResDataType.RD_INT)

        for i in range(len(kw1)):
            kw1[i] = i
            kw2[i] = 2 * i
            kw3[i] = 3 * i

        kw_list = [kw1, kw2, kw2]

        with TestAreaContext("context") as ta:
            createFile("TEST", kw_list)
            gc.collect()
            kw_list2 = loadKeywords("TEST")

            for kw1, kw2 in zip(kw_list, kw_list2):
                self.assertEqual(kw1, kw2)

    def test_broken_file(self):
        with TestAreaContext("test_broken_file"):
            with open("CASE.FINIT", "w") as f:
                f.write(
                    "This - is not a RDISPE file\nsdlcblhcdbjlwhc\naschscbasjhcasc\nascasck c s s aiasic asc"
                )
            f = ResdataFile("CASE.FINIT")
            self.assertEqual(len(f), 0)

            kw = ResdataKW("HEADER", 100, ResDataType.RD_INT)
            with openFortIO("FILE", mode=FortIO.WRITE_MODE) as f:
                kw.fwrite(f)
                kw.fwrite(f)

            with open("FILE", "a+") as f:
                f.write("Seom random gibberish")

            f = ResdataFile("FILE")
            self.assertEqual(len(f), 2)

            with openFortIO("FILE", mode=FortIO.WRITE_MODE) as f:
                kw.fwrite(f)
                kw.fwrite(f)

            file_size = os.path.getsize("FILE")
            with open("FILE", "a+") as f:
                f.truncate(int(file_size * 0.75))

            f = ResdataFile("FILE")
            self.assertEqual(len(f), 1)

    def test_block_view(self):
        with TestAreaContext("python/rd_file/view"):
            with openFortIO("TEST", mode=FortIO.WRITE_MODE) as f:
                for i in range(5):
                    header = ResdataKW("HEADER", 1, ResDataType.RD_INT)
                    header[0] = i

                    data1 = ResdataKW("DATA1", 100, ResDataType.RD_INT)
                    data1.assign(i)

                    data2 = ResdataKW("DATA2", 100, ResDataType.RD_INT)
                    data2.assign(i * 10)

                    header.fwrite(f)
                    data1.fwrite(f)
                    data2.fwrite(f)

            rd_file = ResdataFile("TEST")
            pfx = "ResdataFile("
            self.assertEqual(pfx, repr(rd_file)[: len(pfx)])
            with self.assertRaises(KeyError):
                rd_file.blockView("NO", 1)

            with self.assertRaises(IndexError):
                rd_file.blockView("HEADER", 100)

            with self.assertRaises(IndexError):
                rd_file.blockView("HEADER", 1000)

            bv = rd_file.blockView("HEADER", -1)

            for i in range(5):
                view = rd_file.blockView("HEADER", i)
                self.assertEqual(view.unique_size(), 3)
                self.assertEqual(len(view), 3)
                self.assertEqual(view.unique_kw(), ["HEADER", "DATA1", "DATA2"])
                header = view["HEADER"][0]
                data1 = view["DATA1"][0]
                data2 = view["DATA2"][0]

                self.assertEqual(header[0], i)
                self.assertEqual(data1[99], i)
                self.assertEqual(data2[99], i * 10)

            for i in range(5):
                view = rd_file.blockView2("HEADER", "DATA2", i)
                self.assertEqual(len(view), 2)
                header = view["HEADER"][0]
                data1 = view["DATA1"][0]

                self.assertEqual(header[0], i)
                self.assertEqual(data1[99], i)

                self.assertFalse("DATA2" in view)

            view = rd_file.blockView2("HEADER", None, 0)
            self.assertEqual(len(view), len(rd_file))

            view = rd_file.blockView2(None, "DATA2", 0)


def test_report_list(tmpdir):
    with tmpdir.as_cwd():
        grid = Grid.create_rectangular(dims=(1, 1, 1), dV=(50, 50, 50))
        create_restart(grid, "TEST", [1.0])
        assert ResdataFile.file_report_list("TEST.UNRST") == [10.0]

        with openResdataFile("TEST.UNRST") as rd_file:
            assert rd_file.report_list == [10.0]
            assert (
                rd_file.restart_get_kw("SEQNUM", datetime.datetime(2000, 1, 1)).name
                == "SEQNUM"
            )
            assert rd_file.iget_restart_sim_time(0) == datetime.datetime(2000, 1, 1)
            assert rd_file.iget_restart_sim_days(1) == 0.0
            with openFortIO("TEST2.UNRST", FortIO.WRITE_MODE) as fortio:
                rd_file.fwrite(fortio)
