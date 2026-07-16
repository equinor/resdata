import datetime
import gc
import os.path
import shutil
import struct
from pathlib import Path

import pytest
from resdata import FileMode, FileType, ResDataType
from resdata.grid import GridGenerator
from resdata.resfile import FortIO, ResdataFile, ResdataKW, open_rd_file, openFortIO
from resdata.util.util import CWDContext

from tests import ResdataTest, source_root

from .create_restart import create_restart


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
        file_type, step, fmt_file = ResdataFile.get_filetype(filename)
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
        tmpdir = self.tmp_path_factory.mktemp("python_rd_file_context", numbered=True)
        with self.monkeypatch.context() as mp:
            mp.chdir(tmpdir)
            kw1 = ResdataKW("KW1", 100, ResDataType.RD_INT)
            kw2 = ResdataKW("KW2", 100, ResDataType.RD_INT)
            with openFortIO("TEST", mode=FortIO.WRITE_MODE) as f:
                kw1.fwrite(f)
                kw2.fwrite(f)

            self.assertFalse(ResdataFile.contains_report_step("TEST", 99))
            self.assertFalse(
                ResdataFile.contains_sim_time("TEST", datetime.datetime(2024, 4, 4))
            )
            with open_rd_file("TEST") as rd_file:
                assert rd_file.iget_named_kw("KW1", 0).name == "KW1"
                self.assertEqual(len(rd_file), 2)
                self.assertTrue(rd_file.has_kw("KW1"))
                self.assertTrue(rd_file.has_kw("KW2"))
                self.assertFalse(rd_file.has_report_step(99))
                self.assertFalse(rd_file.has_sim_time(datetime.datetime(2024, 4, 4)))
                self.assertEqual(rd_file[1], rd_file[-1])

                self.assertEqual(rd_file.iget_kw(0).name, "KW1")

    def test_rd_index(self):
        tmpdir = self.tmp_path_factory.mktemp("python_rd_file_context", numbered=True)
        with self.monkeypatch.context() as mp:
            mp.chdir(tmpdir)
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

            rd_file_index = ResdataFile("TEST", index_filename="INDEX_FILE")
            for kw in ["KW1", "KW2", "KW3", "KW4"]:
                self.assertIn(kw, rd_file_index)

            with self.assertRaises(IOError):
                rd_file.write_index("does-not-exist/INDEX")

            with self.assertRaises(IOError):
                rd_file_index = ResdataFile(
                    "TEST", index_filename="index_does_not_exist"
                )

            os.mkdir("path")
            shutil.copyfile("TEST", "path/TEST")
            rd_file = ResdataFile("path/TEST")
            rd_file.write_index("path/index")

            with CWDContext("path"):
                rd_file = ResdataFile("TEST", index_filename="index")

    def test_that_write_index_stays_binary_compatible(self):
        # The index file written by ResdataFile.write_index must remain
        # binary-compatible so that index files committed by earlier versions of
        # resdata keep working.
        grid_file = (
            Path(source_root())
            / "test-data"
            / "local"
            / "ECLIPSE"
            / "faarikaal"
            / "faarikaal1.EGRID"
        )
        snapshot_index = grid_file.with_suffix(grid_file.suffix + ".index")

        tmpdir = self.tmp_path_factory.mktemp(
            "python_rd_file_index_compat", numbered=True
        )
        with self.monkeypatch.context() as mp:
            mp.chdir(tmpdir)
            shutil.copyfile(grid_file, "faarikaal1.EGRID")

            rd_file = ResdataFile("faarikaal1.EGRID")
            rd_file.write_index("faarikaal1.EGRID.index")
            rd_file.close()

            self.assertEqual(
                Path("faarikaal1.EGRID.index").read_bytes(),
                snapshot_index.read_bytes(),
            )
            read_with_index = ResdataFile(
                "faarikaal1.EGRID", index_filename="faarikaal1.EGRID.index"
            )
            assert [
                read_with_index.iget_kw(i).name for i in range(len(read_with_index))
            ] == [
                "FILEHEAD",
                "MAPUNITS",
                "MAPAXES",
                "GRIDUNIT",
                "GRIDHEAD",
                "COORD",
                "ZCORN",
                "ACTNUM",
                "ENDGRID",
                "NNCHEAD",
                "NNC1",
                "NNC2",
            ]

    def test_that_the_index_stores_the_filename_with_extension(self):
        tmpdir = self.tmp_path_factory.mktemp(
            "python_rd_file_index_name", numbered=True
        )
        with self.monkeypatch.context() as mp:
            mp.chdir(tmpdir)
            os.mkdir("sub")
            kw = ResdataKW("KW1", 100, ResDataType.RD_INT)
            with openFortIO("sub/TEST.DAT", mode=FortIO.WRITE_MODE) as f:
                kw.fwrite(f)

            rd_file = ResdataFile("sub/TEST.DAT")
            rd_file.write_index("sub/INDEX_FILE")
            rd_file.close()

            # The index stores only the base filename, keeping the extension but
            # dropping the directory component.
            with open("sub/INDEX_FILE", "rb") as fh:
                (name_len,) = struct.unpack("i", fh.read(4))
                stored_name = fh.read(name_len).decode("ascii")
            self.assertEqual(stored_name, "TEST.DAT")

            # Loading from within the file's directory works even though the
            # index was written with a different directory prefix,
            # because the stored name is directory independent.
            with CWDContext("sub"):
                rd_file_index = ResdataFile("TEST.DAT", index_filename="INDEX_FILE")
                self.assertIn("KW1", rd_file_index)

                # Renaming the file so the extensions differ
                os.rename("TEST.DAT", "TEST.BIN")
                with self.assertRaisesRegex(
                    OSError, "Index file did not contain a valid index"
                ):
                    ResdataFile("TEST.BIN", index_filename="INDEX_FILE")

    def test_that_opening_a_rd_file_with_a_corrupt_index_raises(self):
        tmpdir = self.tmp_path_factory.mktemp("python_rd_file_bad_index", numbered=True)
        with self.monkeypatch.context() as mp:
            mp.chdir(tmpdir)
            kw = ResdataKW("KW1", 100, ResDataType.RD_INT)
            with openFortIO("TEST", mode=FortIO.WRITE_MODE) as f:
                kw.fwrite(f)

            rd_file = ResdataFile("TEST")
            rd_file.write_index("INDEX_FILE")
            rd_file.close()

            # The index file stores the source filename followed by the number
            # of keywords. Overwriting that count with a negative value keeps the
            # index looking valid (name still matches) but makes reading the file
            # view fail.
            with open("INDEX_FILE", "r+b") as fh:
                (name_len,) = struct.unpack("i", fh.read(4))
                fh.seek(4 + name_len + 1)
                fh.write(struct.pack("i", -1))

            with self.assertRaisesRegex(OSError, "Invalid index size: -1"):
                ResdataFile("TEST", index_filename="INDEX_FILE")

    def test_that_opening_a_rd_file_with_a_truncated_index_raises(self):
        tmpdir = self.tmp_path_factory.mktemp(
            "python_rd_file_short_index", numbered=True
        )
        with self.monkeypatch.context() as mp:
            mp.chdir(tmpdir)
            kw = ResdataKW("KW1", 100, ResDataType.RD_INT)
            with openFortIO("TEST", mode=FortIO.WRITE_MODE) as f:
                kw.fwrite(f)

            rd_file = ResdataFile("TEST")
            rd_file.write_index("INDEX_FILE")
            rd_file.close()

            # Keep the source-filename string and the (positive) keyword count,
            # but drop the keyword headers that follow so reading them fails.
            with open("INDEX_FILE", "r+b") as fh:
                (name_len,) = struct.unpack("i", fh.read(4))
                fh.truncate(4 + name_len + 1 + 4)

            with self.assertRaises(OSError):
                ResdataFile("TEST", index_filename="INDEX_FILE")

    def test_that_opening_a_rd_file_with_an_empty_index_raises(self):
        tmpdir = self.tmp_path_factory.mktemp(
            "python_rd_file_empty_index", numbered=True
        )
        with self.monkeypatch.context() as mp:
            mp.chdir(tmpdir)
            kw = ResdataKW("KW1", 100, ResDataType.RD_INT)
            with openFortIO("TEST", mode=FortIO.WRITE_MODE) as f:
                kw.fwrite(f)

            # An empty index file cannot contain a valid index.
            open("INDEX_FILE", "wb").close()

            with self.assertRaisesRegex(
                OSError, 'Index file did not contain a valid index for "TEST"'
            ):
                ResdataFile("TEST", index_filename="INDEX_FILE")

    def test_that_opening_a_rd_file_with_a_number_only_index_raises(self):
        tmpdir = self.tmp_path_factory.mktemp(
            "python_rd_file_number_index", numbered=True
        )
        with self.monkeypatch.context() as mp:
            mp.chdir(tmpdir)
            kw = ResdataKW("KW1", 100, ResDataType.RD_INT)
            with openFortIO("TEST", mode=FortIO.WRITE_MODE) as f:
                kw.fwrite(f)

            # An index file containing only a 4 byte integer is not a valid
            # index: the integer is read as the length of the stored source
            # filename, but there are no following bytes to read.
            with open("INDEX_FILE", "wb") as fh:
                fh.write(struct.pack("i", 16))

            with self.assertRaisesRegex(
                OSError, 'Index file did not contain a valid index for "TEST"'
            ):
                ResdataFile("TEST", index_filename="INDEX_FILE")

    def test_that_opening_a_rd_file_truncated_within_a_keyword_header_raises(self):
        tmpdir = self.tmp_path_factory.mktemp(
            "python_rd_file_partial_header", numbered=True
        )
        with self.monkeypatch.context() as mp:
            mp.chdir(tmpdir)
            kw = ResdataKW("KW1", 100, ResDataType.RD_INT)
            with openFortIO("TEST", mode=FortIO.WRITE_MODE) as f:
                kw.fwrite(f)

            rd_file = ResdataFile("TEST")
            rd_file.write_index("INDEX_FILE")
            rd_file.close()

            # Keep the source-filename string and the (positive) keyword count,
            # then cut off partway through the first keyword header so only a
            # handful of its bytes remain.
            with open("INDEX_FILE", "r+b") as fh:
                (name_len,) = struct.unpack("i", fh.read(4))
                fh.truncate(4 + name_len + 1 + 4 + 10)

            with self.assertRaises(OSError):
                ResdataFile("TEST", index_filename="INDEX_FILE")

    def test_that_writing_an_index_to_a_read_only_location_raises(self):
        if hasattr(os, "geteuid") and os.geteuid() == 0:
            self.skipTest("cannot test permission errors as root")

        tmpdir = self.tmp_path_factory.mktemp("python_rd_file_ro_index", numbered=True)
        with self.monkeypatch.context() as mp:
            mp.chdir(tmpdir)
            kw = ResdataKW("KW1", 100, ResDataType.RD_INT)
            with openFortIO("TEST", mode=FortIO.WRITE_MODE) as f:
                kw.fwrite(f)

            os.mkdir("read_only")
            os.chmod("read_only", 0o500)
            try:
                rd_file = ResdataFile("TEST")
                with self.assertRaises(OSError):
                    rd_file.write_index("read_only/INDEX_FILE")
            finally:
                os.chmod("read_only", 0o700)

    def test_that_opening_a_non_existing_file_raises(self):
        tmpdir = self.tmp_path_factory.mktemp("python_rd_file_missing", numbered=True)
        with self.monkeypatch.context() as mp:
            mp.chdir(tmpdir)
            with self.assertRaisesRegex(OSError, "File NO_SUCH_FILE does not exist"):
                ResdataFile("NO_SUCH_FILE")

    def test_that_opening_a_non_existing_file_writable_raises(self):
        tmpdir = self.tmp_path_factory.mktemp(
            "python_rd_file_missing_rw", numbered=True
        )
        with self.monkeypatch.context() as mp:
            mp.chdir(tmpdir)
            with self.assertRaisesRegex(
                OSError, "Failed to open FortIO file NO_SUCH_FILE"
            ):
                ResdataFile("NO_SUCH_FILE", flags=FileMode.WRITABLE)

    def test_that_fast_opening_with_a_missing_source_file_raises(self):
        tmpdir = self.tmp_path_factory.mktemp(
            "python_rd_file_fast_no_source", numbered=True
        )
        with self.monkeypatch.context() as mp:
            mp.chdir(tmpdir)
            kw = ResdataKW("KW1", 100, ResDataType.RD_INT)
            with openFortIO("TEST", mode=FortIO.WRITE_MODE) as f:
                kw.fwrite(f)

            rd_file = ResdataFile("TEST")
            rd_file.write_index("INDEX_FILE")
            rd_file.close()

            with self.assertRaisesRegex(OSError, 'File "NO_SUCH_FILE" does not exist'):
                ResdataFile("NO_SUCH_FILE", index_filename="INDEX_FILE")

    def test_that_fast_opening_with_a_missing_index_file_raises(self):
        tmpdir = self.tmp_path_factory.mktemp(
            "python_rd_file_fast_no_index", numbered=True
        )
        with self.monkeypatch.context() as mp:
            mp.chdir(tmpdir)
            kw = ResdataKW("KW1", 100, ResDataType.RD_INT)
            with openFortIO("TEST", mode=FortIO.WRITE_MODE) as f:
                kw.fwrite(f)

            with self.assertRaisesRegex(OSError, 'File "NO_SUCH_INDEX" does not exist'):
                ResdataFile("TEST", index_filename="NO_SUCH_INDEX")

    def test_that_fast_opening_with_a_stale_index_raises(self):
        tmpdir = self.tmp_path_factory.mktemp(
            "python_rd_file_stale_index", numbered=True
        )
        with self.monkeypatch.context() as mp:
            mp.chdir(tmpdir)
            kw = ResdataKW("KW1", 100, ResDataType.RD_INT)
            with openFortIO("TEST", mode=FortIO.WRITE_MODE) as f:
                kw.fwrite(f)

            rd_file = ResdataFile("TEST")
            rd_file.write_index("INDEX_FILE")
            rd_file.close()

            # Make the source file newer than its index so the index is stale.
            index_stat = os.stat("INDEX_FILE")
            os.utime("TEST", (index_stat.st_atime + 10, index_stat.st_mtime + 10))

            with self.assertRaisesRegex(
                OSError, 'The file "TEST" is newer than its index file'
            ):
                ResdataFile("TEST", index_filename="INDEX_FILE")

    def test_that_fast_opening_with_a_foreign_index_raises(self):
        tmpdir = self.tmp_path_factory.mktemp(
            "python_rd_file_foreign_index", numbered=True
        )
        with self.monkeypatch.context() as mp:
            mp.chdir(tmpdir)
            kw = ResdataKW("KW1", 100, ResDataType.RD_INT)
            with openFortIO("TEST", mode=FortIO.WRITE_MODE) as f:
                kw.fwrite(f)
            with openFortIO("OTHER", mode=FortIO.WRITE_MODE) as f:
                kw.fwrite(f)

            # Build an index that stores "OTHER" as its source filename and use
            # it to open "TEST"; the stored name does not match the file.
            other = ResdataFile("OTHER")
            other.write_index("OTHER_INDEX")
            other.close()

            with self.assertRaisesRegex(
                OSError, 'Index file did not contain a valid index for "TEST"'
            ):
                ResdataFile("TEST", index_filename="OTHER_INDEX")

    def test_save_kw(self):
        tmpdir = self.tmp_path_factory.mktemp("python_rd_file_save_kw", numbered=True)
        with self.monkeypatch.context() as mp:
            mp.chdir(tmpdir)
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

        ta = self.tmp_path_factory.mktemp("context", numbered=True)
        with self.monkeypatch.context() as mp:
            mp.chdir(ta)
            createFile("TEST", kw_list)
            gc.collect()
            kw_list2 = loadKeywords("TEST")

            for kw1, kw2 in zip(kw_list, kw_list2):
                self.assertEqual(kw1, kw2)

    def test_broken_file(self):
        tmpdir = self.tmp_path_factory.mktemp("test_broken_file", numbered=True)
        with self.monkeypatch.context() as mp:
            mp.chdir(tmpdir)
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
        tmpdir = self.tmp_path_factory.mktemp("python_rd_file_view", numbered=True)
        with self.monkeypatch.context() as mp:
            mp.chdir(tmpdir)
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
                rd_file.block_view("NO", 1)

            with self.assertRaises(IndexError):
                rd_file.block_view("HEADER", 100)

            with self.assertRaises(IndexError):
                rd_file.block_view("HEADER", 1000)

            bv = rd_file.block_view("HEADER", -1)

            for i in range(5):
                view = rd_file.block_view("HEADER", i)
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
                view = rd_file.block_view2("HEADER", "DATA2", i)
                self.assertEqual(len(view), 2)
                header = view["HEADER"][0]
                data1 = view["DATA1"][0]

                self.assertEqual(header[0], i)
                self.assertEqual(data1[99], i)

                self.assertFalse("DATA2" in view)

            view = rd_file.block_view2("HEADER", None, 0)
            self.assertEqual(len(view), len(rd_file))

            view = rd_file.block_view2(None, "DATA2", 0)

            with pytest.raises(KeyError, match="The keyword:FAULTY is not in file"):
                rd_file.block_view2("HEADER", "FAULTY", 0)


def test_report_list(tmpdir):
    with tmpdir.as_cwd():
        grid = GridGenerator.create_rectangular(dims=(1, 1, 1), dV=(50, 50, 50))
        create_restart(grid, "TEST", [1.0])
        assert ResdataFile.file_report_list("TEST.UNRST") == [10.0]

        with open_rd_file("TEST.UNRST") as rd_file:
            assert rd_file.report_list == [10.0]
            assert (
                rd_file.restart_get_kw("SEQNUM", datetime.datetime(2000, 1, 1)).name
                == "SEQNUM"
            )
            assert rd_file.report_steps == [10]
            assert rd_file.iget_restart_sim_time(0) == datetime.datetime(2000, 1, 1)
            assert rd_file.iget_restart_sim_days(1) == 0.0
            with openFortIO("TEST2.UNRST", FortIO.WRITE_MODE) as fortio:
                rd_file.fwrite(fortio)


def test_opening_non_existing_file_throws(tmpdir):
    with pytest.raises(OSError):
        ResdataFile("non_existing_file")


def test_keys_returns_unique_kw_names(tmpdir):
    with tmpdir.as_cwd():
        kw1 = ResdataKW("AAA", 3, ResDataType.RD_INT)
        kw2 = ResdataKW("BBB", 3, ResDataType.RD_INT)
        kw3 = ResdataKW("AAA", 5, ResDataType.RD_INT)
        with openFortIO("TEST.UNRST", mode=FortIO.WRITE_MODE) as f:
            kw1.fwrite(f)
            kw2.fwrite(f)
            kw3.fwrite(f)

        rd_file = ResdataFile("TEST.UNRST")
        keys = list(rd_file.keys())
        assert set(keys) == {"AAA", "BBB"}


def test_report_list_non_unified_filename(tmpdir):
    with tmpdir.as_cwd():
        kw = ResdataKW("HEADER", 1, ResDataType.RD_INT)
        kw[0] = 0
        with openFortIO("CASE.X0042", mode=FortIO.WRITE_MODE) as f:
            kw.fwrite(f)

        rd_file = ResdataFile("CASE.X0042")
        assert rd_file.report_list == [42]


def _write_single_kw_file(filename, kw_name="MY_KEY", size=5):
    kw = ResdataKW(kw_name, size, ResDataType.RD_INT)
    for index in range(size):
        kw[index] = index
    with openFortIO(filename, mode=FortIO.WRITE_MODE) as f:
        kw.fwrite(f)
    return kw


@pytest.mark.parametrize("extra_flags", [FileMode.DEFAULT, FileMode.CLOSE_STREAM])
def test_save_kw_roundtrip_with_own_kw(tmpdir, extra_flags):
    with tmpdir.as_cwd():
        _write_single_kw_file("TEST")

        rd_file = ResdataFile("TEST", flags=FileMode.WRITABLE | extra_flags)
        loaded_kw = rd_file["MY_KEY"][0]
        loaded_kw[0] = 42

        rd_file.save_kw(loaded_kw)
        rd_file.close()

        assert ResdataFile("TEST")["MY_KEY"][0][0] == 42


@pytest.mark.parametrize("extra_flags", [FileMode.DEFAULT, FileMode.CLOSE_STREAM])
def test_fast_open_with_index_roundtrip(tmpdir, extra_flags):
    with tmpdir.as_cwd():
        _write_single_kw_file("TEST")

        rd_file = ResdataFile("TEST")
        rd_file.write_index("INDEX_FILE")
        rd_file.close()

        fast_opened = ResdataFile(
            "TEST", flags=extra_flags, index_filename="INDEX_FILE"
        )
        assert "MY_KEY" in fast_opened
        assert list(fast_opened["MY_KEY"][0]) == [0, 1, 2, 3, 4]


def test_save_kw_with_kw_from_different_file_raises(tmpdir):
    with tmpdir.as_cwd():
        _write_single_kw_file("A")
        _write_single_kw_file("B")

        writable = ResdataFile("A", flags=FileMode.WRITABLE)
        other = ResdataFile("B")
        foreign_kw = other["MY_KEY"][0]

        with pytest.raises(IndexError):
            writable.save_kw(foreign_kw)


def test_save_kw_with_standalone_kw_raises(tmpdir):
    with tmpdir.as_cwd():
        _write_single_kw_file("TEST")

        writable = ResdataFile("TEST", flags=FileMode.WRITABLE)

        standalone_kw = ResdataKW("MY_KEY", 5, ResDataType.RD_INT)
        for index in range(5):
            standalone_kw[index] = index

        with pytest.raises(IndexError):
            writable.save_kw(standalone_kw)


def test_report_steps_on_non_restart_file_returns_empty_list(tmpdir):
    with tmpdir.as_cwd():
        _write_single_kw_file("TEST")

        rd_file = ResdataFile("TEST")

        assert rd_file.report_steps == []


def test_num_report_steps_on_non_restart_file_returns_zero(tmpdir):
    with tmpdir.as_cwd():
        _write_single_kw_file("TEST")

        rd_file = ResdataFile("TEST")

        assert rd_file.num_report_steps() == 0


def test_report_dates_on_non_restart_file_returns_none(tmpdir):
    with tmpdir.as_cwd():
        _write_single_kw_file("TEST")

        rd_file = ResdataFile("TEST")

        assert rd_file.report_dates is None


def test_save_kw_on_read_only_file_raises(tmpdir):
    with tmpdir.as_cwd():
        _write_single_kw_file("TEST")

        read_only = ResdataFile("TEST")
        loaded_kw = read_only["MY_KEY"][0]

        with pytest.raises(OSError, match="opened read only"):
            read_only.save_kw(loaded_kw)


def _restart_kw(name, dtype, values):
    kw = ResdataKW(name, len(values), dtype)
    for index, value in enumerate(values):
        kw[index] = value
    return kw


@pytest.fixture
def block_view_file(tmp_path):
    """A file laid out as: SEQNUM PRESSURE SWAT PRESSURE SWAT."""
    path = str(tmp_path / "TEST")
    with openFortIO(path, mode=FortIO.WRITE_MODE) as f:
        _restart_kw("SEQNUM", ResDataType.RD_INT, [10]).fwrite(f)
        _restart_kw("PRESSURE", ResDataType.RD_FLOAT, [1.0, 2.0, 3.0]).fwrite(f)
        _restart_kw("SWAT", ResDataType.RD_FLOAT, [0.1, 0.2, 0.3]).fwrite(f)
        _restart_kw("PRESSURE", ResDataType.RD_FLOAT, [4.0, 5.0, 6.0]).fwrite(f)
        _restart_kw("SWAT", ResDataType.RD_FLOAT, [0.4, 0.5, 0.6]).fwrite(f)
    return path


def _kw_names(view):
    return [view[i].name for i in range(len(view))]


def test_block_view_returns_block_up_to_next_occurrence(block_view_file):
    with open_rd_file(block_view_file) as rd_file:
        block = rd_file.block_view("PRESSURE", 0)
        assert _kw_names(block) == ["PRESSURE", "SWAT"]


def test_block_view_last_block_extends_to_end_of_file(block_view_file):
    with open_rd_file(block_view_file) as rd_file:
        block = rd_file.block_view("PRESSURE", 1)
        assert _kw_names(block) == ["PRESSURE", "SWAT"]


def test_block_view_with_negative_index_counts_from_end(block_view_file):
    with open_rd_file(block_view_file) as rd_file:
        block = rd_file.block_view("PRESSURE", -1)
        assert list(block["PRESSURE"][0]) == pytest.approx([4.0, 5.0, 6.0])


def test_block_view_with_unknown_keyword_raises_keyerror(block_view_file):
    with open_rd_file(block_view_file) as rd_file:
        with pytest.raises(KeyError):
            rd_file.block_view("NOSUCHKW", 0)


def test_block_view_with_out_of_range_index_raises_indexerror(block_view_file):
    with open_rd_file(block_view_file) as rd_file:
        with pytest.raises(IndexError):
            rd_file.block_view("PRESSURE", 5)


def test_block_view2_with_none_start_kw_reads_from_start_of_file(block_view_file):
    with open_rd_file(block_view_file) as rd_file:
        block = rd_file.block_view2(None, "PRESSURE", 0)
        assert _kw_names(block) == ["SEQNUM"]


def test_block_view2_with_none_stop_kw_reads_to_end_of_file(block_view_file):
    with open_rd_file(block_view_file) as rd_file:
        block = rd_file.block_view2("SEQNUM", None, 0)
        assert _kw_names(block) == ["SEQNUM", "PRESSURE", "SWAT", "PRESSURE", "SWAT"]


def test_block_view2_with_none_start_and_stop_returns_all_keywords(block_view_file):
    with open_rd_file(block_view_file) as rd_file:
        block = rd_file.block_view2(None, None, 0)
        assert _kw_names(block) == ["SEQNUM", "PRESSURE", "SWAT", "PRESSURE", "SWAT"]


def test_block_view2_with_unknown_stop_keyword_raises_keyerror(block_view_file):
    with open_rd_file(block_view_file) as rd_file:
        with pytest.raises(KeyError, match="The keyword:FAULTY is not in file"):
            rd_file.block_view2("SEQNUM", "FAULTY", 0)


def _restart_intehead(day, month, year):
    header = ResdataKW("INTEHEAD", 67, ResDataType.RD_INT)
    header[64] = day
    header[65] = month
    header[66] = year
    return header


def _write_restart_file(path):
    """Write a minimal, valid restart file with two report steps.

    Layout per block: SEQNUM, INTEHEAD, PRESSURE, SWAT. The two INTEHEADs
    encode the simulation dates 2000-01-01 and 2010-01-01 so that the
    date/restart lookups return meaningful values. Two blocks are written so
    that the first block can be extracted with an explicit ``end_kw`` of
    ``SEQNUM`` (working around bug #1247 in ``block_view``, which mishandles a
    ``None`` end keyword).
    """
    with openFortIO(path, mode=FortIO.WRITE_MODE) as f:
        _restart_kw("SEQNUM", ResDataType.RD_INT, [10]).fwrite(f)
        _restart_intehead(1, 1, 2000).fwrite(f)
        _restart_kw("PRESSURE", ResDataType.RD_FLOAT, [1.0, 2.0, 3.0]).fwrite(f)
        _restart_kw("SWAT", ResDataType.RD_FLOAT, [0.1, 0.2, 0.3]).fwrite(f)
        _restart_kw("SEQNUM", ResDataType.RD_INT, [20]).fwrite(f)
        _restart_intehead(1, 1, 2010).fwrite(f)
        _restart_kw("PRESSURE", ResDataType.RD_FLOAT, [4.0, 5.0, 6.0]).fwrite(f)
        _restart_kw("SWAT", ResDataType.RD_FLOAT, [0.4, 0.5, 0.6]).fwrite(f)


@pytest.fixture
def objects_from_closed_file(tmp_path):
    """Return (rd_file, view, kw) read inside an ``open_rd_file`` context that
    has since been closed.
    """
    path = str(tmp_path / "R.UNRST")
    _write_restart_file(path)

    with open_rd_file(path) as rd_file:
        # The first restart block is delimited by the next SEQNUM keyword
        view = rd_file.block_view2("SEQNUM", "SEQNUM", 0)
        kw = view["PRESSURE"][0]
        assert list(kw) == pytest.approx([1.0, 2.0, 3.0])

    # The context manager has now closed the file
    return rd_file, view, kw


def test_that_resdata_file_is_usable_after_close(objects_from_closed_file):
    rd_file, _, _ = objects_from_closed_file

    assert rd_file
    assert rd_file.get_filename().endswith("R.UNRST")
    assert len(rd_file) == 8
    assert rd_file.size == 8
    assert rd_file.unique_size == 4
    assert set(rd_file.keys()) == {"SEQNUM", "INTEHEAD", "PRESSURE", "SWAT"}
    assert len(rd_file.headers) == 8

    assert rd_file.has_kw("PRESSURE")
    assert "SWAT" in rd_file
    assert rd_file.num_named_kw("PRESSURE") == 2

    assert rd_file.iget_kw(2).name == "PRESSURE"
    assert rd_file.iget_named_kw("SWAT", 0).name == "SWAT"
    assert list(rd_file["PRESSURE"][0]) == pytest.approx([1.0, 2.0, 3.0])

    assert rd_file.report_steps == [10, 20]
    assert rd_file.report_list == [10, 20]
    assert rd_file.num_report_steps() == 2
    assert rd_file.has_report_step(10)
    assert not rd_file.has_report_step(99)
    assert rd_file.has_sim_time(datetime.datetime(2000, 1, 1))
    assert rd_file.dates == [
        datetime.datetime(2000, 1, 1),
        datetime.datetime(2010, 1, 1),
    ]

    assert len(rd_file.block_view2("SEQNUM", "SEQNUM", 0)) == 4
    assert len(rd_file.restart_view(report_step=10)) >= 1


def test_that_resdata_file_view_is_usable_after_close(
    objects_from_closed_file,
):
    _, view, _ = objects_from_closed_file

    assert "ResdataFileView" in repr(view)
    assert len(view) == 4
    assert "PRESSURE" in view
    assert view.num_keywords("PRESSURE") == 1
    assert view.unique_size() == 4
    assert set(view.unique_kw()) == {"SEQNUM", "INTEHEAD", "PRESSURE", "SWAT"}

    assert view.iget_named_kw("PRESSURE", 0).name == "PRESSURE"
    assert view[2].name == "PRESSURE"

    # reading SWAT now forces a lazy reopen
    assert list(view["SWAT"][0]) == pytest.approx([0.1, 0.2, 0.3])

    assert "PRESSURE" in view.block_view2("PRESSURE", "SWAT", 0)
    assert view.block_view("PRESSURE", 0)["PRESSURE"][0].name == "PRESSURE"
    assert len(view.restart_view(report_step=10)) >= 1


def test_that_resdata_kw_is_usable_after_context_close(objects_from_closed_file):
    _, _, kw = objects_from_closed_file

    assert kw.name == "PRESSURE"
    assert kw.get_name() == "PRESSURE"
    assert len(kw) == 3
    assert kw[0] == pytest.approx(1.0)
    assert list(kw) == pytest.approx([1.0, 2.0, 3.0])

    assert kw.data_type.is_float()
    assert kw.header == ("PRESSURE", 3, kw.type_name())

    assert kw.get_min() == pytest.approx(1.0)
    assert kw.get_max() == pytest.approx(3.0)
    assert kw.get_min_max() == pytest.approx((1.0, 3.0))
    assert kw.sum() == pytest.approx(6.0)

    assert list(kw.numpy_view()) == pytest.approx([1.0, 2.0, 3.0])
    assert list(kw.numpy_copy()) == pytest.approx([1.0, 2.0, 3.0])

    kw_copy = kw.copy()
    assert kw.equal(kw_copy)
    assert kw == kw_copy
    assert "PRESSURE" in str(kw)


def test_that_kws_are_valid_after_closing_the_file(tmp_path):
    path = str(tmp_path / "R.UNRST")
    _write_restart_file(path)

    rd_file = ResdataFile(path)
    kw = rd_file["PRESSURE"][0]
    assert list(kw) == pytest.approx([1.0, 2.0, 3.0])

    rd_file.close()

    assert kw.name == "PRESSURE"
    assert list(kw) == pytest.approx([1.0, 2.0, 3.0])


def test_report_list_on_non_restart_file_raises_type_error(tmpdir):
    with tmpdir.as_cwd():
        kw = ResdataKW("HEADER", 1, ResDataType.RD_INT)
        kw[0] = 0
        with openFortIO("CASE.INIT", mode=FortIO.WRITE_MODE) as f:
            kw.fwrite(f)

        rd_file = ResdataFile("CASE.INIT")
        with pytest.raises(TypeError, match="which is not a restart file"):
            rd_file.report_list


def test_iget_kw_with_copy_returns_independent_copy(tmpdir):
    with tmpdir.as_cwd():
        kw = _write_single_kw_file("CASE.INIT", kw_name="MY_KEY", size=5)

        rd_file = ResdataFile("CASE.INIT")
        copied = rd_file.iget_kw(0, copy=True)
        assert copied.name == "MY_KEY"
        assert list(copied) == list(kw)

        # The copy is independent of the file-owned keyword.
        copied[0] = 42
        assert rd_file.iget_kw(0)[0] == 0


def test_restart_get_kw_with_copy_returns_independent_copy(tmpdir):
    with tmpdir.as_cwd():
        grid = GridGenerator.create_rectangular(dims=(1, 1, 1), dV=(50, 50, 50))
        create_restart(grid, "TEST", [1.0])

        with open_rd_file("TEST.UNRST") as rd_file:
            copied = rd_file.restart_get_kw(
                "PRESSURE", datetime.datetime(2000, 1, 1), copy=True
            )
            assert copied.name == "PRESSURE"

            original_value = rd_file.restart_get_kw(
                "PRESSURE", datetime.datetime(2000, 1, 1)
            )[0]
            copied[0] = original_value + 1.0
            assert (
                rd_file.restart_get_kw("PRESSURE", datetime.datetime(2000, 1, 1))[0]
                == original_value
            )


def test_restart_get_kw_with_missing_time_raises_index_error(tmpdir):
    with tmpdir.as_cwd():
        grid = GridGenerator.create_rectangular(dims=(1, 1, 1), dV=(50, 50, 50))
        create_restart(grid, "TEST", [1.0])

        with open_rd_file("TEST.UNRST") as rd_file:
            with pytest.raises(IndexError, match="at time"):
                rd_file.restart_get_kw("PRESSURE", datetime.datetime(1999, 1, 1))


def test_restart_get_kw_with_unknown_keyword_raises_key_error(tmpdir):
    with tmpdir.as_cwd():
        grid = GridGenerator.create_rectangular(dims=(1, 1, 1), dV=(50, 50, 50))
        create_restart(grid, "TEST", [1.0])

        with open_rd_file("TEST.UNRST") as rd_file:
            with pytest.raises(KeyError, match="not recognized"):
                rd_file.restart_get_kw("NOSUCHKW", datetime.datetime(2000, 1, 1))


def test_restart_get_kw_for_keyword_missing_at_time_raises_index_error(tmpdir):
    with tmpdir.as_cwd():
        grid = GridGenerator.create_rectangular(dims=(1, 1, 1), dV=(50, 50, 50))
        # RPORV is only written for the first report step (2000), not the second.
        create_restart(grid, "TEST", [1.0], p2=[2.0], rporv1=[3.0])

        with open_rd_file("TEST.UNRST") as rd_file:
            assert rd_file.has_kw("RPORV")
            with pytest.raises(IndexError, match="at time"):
                rd_file.restart_get_kw("RPORV", datetime.datetime(2010, 1, 1))


def test_report_dates_falls_back_to_intehead_without_seqnum(tmpdir):
    with tmpdir.as_cwd():
        intehead = ResdataKW("INTEHEAD", 67, ResDataType.RD_INT)
        intehead[64] = 1  # day
        intehead[65] = 1  # month
        intehead[66] = 2000  # year
        with openFortIO("CASE.INIT", mode=FortIO.WRITE_MODE) as f:
            intehead.fwrite(f)

        rd_file = ResdataFile("CASE.INIT")
        assert not rd_file.has_kw("SEQNUM")
        assert rd_file.report_dates == [datetime.datetime(2000, 1, 1)]


def test_report_dates_without_seqnum_or_intehead_returns_none(tmpdir):
    with tmpdir.as_cwd():
        kw = ResdataKW("HEADER", 1, ResDataType.RD_INT)
        kw[0] = 0
        with openFortIO("CASE.INIT", mode=FortIO.WRITE_MODE) as f:
            kw.fwrite(f)

        rd_file = ResdataFile("CASE.INIT")
        assert rd_file.report_dates is None
