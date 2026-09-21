#!/usr/bin/env python
import os
from random import randint

import pytest
from resdata import ResDataType
from resdata._rd_util import FileMode
from resdata.resfile import FortIO, ResdataFile, ResdataKW, openFortIO

from tests import ResdataTest


class FortIOTest(ResdataTest):
    def test_open_write(self):
        tmpdir = self.tmp_path_factory.mktemp("python_fortio_write", numbered=True)
        with self.monkeypatch.context() as mp:
            mp.chdir(tmpdir)
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

        tmpdir = self.tmp_path_factory.mktemp("python_fortio_write-kw", numbered=True)
        with self.monkeypatch.context() as mp:
            mp.chdir(tmpdir)
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

        t = self.tmp_path_factory.mktemp("python_fortio_ftruncate", numbered=True)
        with self.monkeypatch.context() as mp:
            mp.chdir(t)
            with openFortIO("file", mode=FortIO.WRITE_MODE) as f:
                kw1.fwrite(f)
                pos1 = f.get_position()
                kw2.fwrite(f)

            # Truncate file in read mode; should fail hard.
            with openFortIO("file") as f:
                with self.assertRaises(IOError):
                    f.truncate()

            with openFortIO("file", mode=FortIO.READ_AND_WRITE_MODE) as f:
                f.seek(pos1)
                f.truncate()

            with openFortIO("file", mode=FortIO.READ_MODE) as f:
                with pytest.raises(ValueError, match="invalid whence"):
                    f.seek(pos1, whence=2000)

            f = ResdataFile("file")
            self.assertEqual(len(f), 1)
            kw1_ = f[0]
            self.assertEqual(kw1, kw1_)

    def test_fortio_creation(self):
        tmpdir = self.tmp_path_factory.mktemp("python_fortio_create", numbered=True)
        with self.monkeypatch.context() as mp:
            mp.chdir(tmpdir)
            w = FortIO("test", FortIO.WRITE_MODE)
            rw = FortIO("test", FortIO.READ_AND_WRITE_MODE)
            r = FortIO("test", FortIO.READ_MODE)
            a = FortIO("test", FortIO.APPEND_MODE)

            w.close()
            w.close()  # should not fail

    def test_context(self):
        t = self.tmp_path_factory.mktemp("python_fortio_context", numbered=True)
        with self.monkeypatch.context() as mp:
            mp.chdir(t)
            kw1 = ResdataKW("KW", 2456, ResDataType.RD_FLOAT)
            for i in range(len(kw1)):
                kw1[i] = randint(0, 1000)

            with openFortIO("file", mode=FortIO.WRITE_MODE) as f:
                kw1.fwrite(f)
                self.assertEqual(f.filename(), "file")

            with openFortIO("file") as f:
                kw2 = ResdataKW.fread(f)

            self.assertTrue(kw1 == kw2)

    def test_context_propagates_exceptions(self):
        tmpdir = self.tmp_path_factory.mktemp(
            "python_fortio_context-exception", numbered=True
        )
        with self.monkeypatch.context() as mp:
            mp.chdir(tmpdir)
            with pytest.raises(ValueError):
                with openFortIO("file", mode=FortIO.WRITE_MODE, fmt_file=False) as f:
                    raise ValueError()

    def test_is_fortran_file(self):
        tmpdir = self.tmp_path_factory.mktemp("python_fortio_guess", numbered=True)
        with self.monkeypatch.context() as mp:
            mp.chdir(tmpdir)
            kw1 = ResdataKW("KW", 12345, ResDataType.RD_FLOAT)
            with openFortIO("fortran_file", mode=FortIO.WRITE_MODE) as f:
                kw1.fwrite(f)

            with open("text_file", "w") as f:
                kw1.write_grdecl(f)

            self.assertTrue(FortIO.is_fortran_file("fortran_file"))
            self.assertFalse(FortIO.is_fortran_file("text_file"))


def _int_kw(name, values):
    kw = ResdataKW(name, len(values), ResDataType.RD_INT)
    for index, value in enumerate(values):
        kw[index] = value
    return kw


def _write_two_keywords(path):
    """Writes two INTE keywords and returns the offset of the second one."""
    with openFortIO(str(path), mode=FortIO.WRITE_MODE) as fortio:
        _int_kw("KW1", [0, 1, 2, 3]).fwrite(fortio)
        offset = fortio.get_position()
        _int_kw("KW2", [4, 5, 6, 7]).fwrite(fortio)
    return offset


def test_that_is_fortran_file_raises_when_the_file_cannot_be_opened(tmp_path):
    with pytest.raises(RuntimeError, match="failed to open file"):
        FortIO.is_fortran_file(str(tmp_path / "does_not_exist"))


def test_that_is_fortran_file_with_the_opposite_endianness_rejects_the_file(tmp_path):
    path = tmp_path / "file"
    _write_two_keywords(path)

    assert FortIO.is_fortran_file(str(path))
    assert not FortIO.is_fortran_file(str(path), endian_flip=False)


def test_that_the_position_after_writing_equals_the_file_size(tmp_path):
    path = tmp_path / "file"
    offset = _write_two_keywords(path)

    assert 0 < offset < path.stat().st_size


def test_that_seeking_beyond_the_end_of_a_read_only_file_fails(tmp_path):
    path = tmp_path / "file"
    _write_two_keywords(path)

    with openFortIO(str(path)) as fortio:
        assert not fortio.seek(path.stat().st_size + 1)


def test_that_a_read_only_file_can_be_positioned_relative_to_the_end(tmp_path):
    path = tmp_path / "file"
    _write_two_keywords(path)
    size = path.stat().st_size

    with openFortIO(str(path)) as fortio:
        assert fortio.seek(0, whence=2)
        assert fortio.get_position() == size


def test_that_a_read_only_file_can_be_positioned_relative_to_the_current_position(
    tmp_path,
):
    path = tmp_path / "file"
    offset = _write_two_keywords(path)

    with openFortIO(str(path)) as fortio:
        assert fortio.seek(offset)
        assert fortio.seek(-offset, whence=1)
        assert fortio.get_position() == 0


def test_that_a_writable_file_can_be_positioned_beyond_the_end(tmp_path):
    path = tmp_path / "file"
    _write_two_keywords(path)
    beyond_end = path.stat().st_size + 1024

    with openFortIO(str(path), mode=FortIO.READ_AND_WRITE_MODE) as fortio:
        assert fortio.seek(beyond_end)
        assert fortio.get_position() == beyond_end


def test_that_reading_a_record_with_a_mismatching_trailer_fails(tmp_path):
    path = tmp_path / "file"
    offset = _write_two_keywords(path)
    content = bytearray(path.read_bytes())
    # The trailer of the first data record is the last four bytes
    # before the start of the second keyword.
    content[offset - 4 : offset] = (999).to_bytes(4, "big")
    path.write_bytes(bytes(content))

    with openFortIO(str(path)) as fortio:
        with pytest.raises(ValueError, match="Failed to create ResdataKW instance"):
            ResdataKW.fread(fortio)


def test_that_reading_a_truncated_record_fails(tmp_path):
    path = tmp_path / "file"
    _write_two_keywords(path)
    path.write_bytes(path.read_bytes()[:-8])

    with openFortIO(str(path)) as fortio:
        ResdataKW.fread(fortio)
        with pytest.raises(ValueError, match="Failed to create ResdataKW instance"):
            ResdataKW.fread(fortio)


def test_that_truncating_to_an_explicit_size_removes_trailing_keywords(tmp_path):
    path = tmp_path / "file"
    offset = _write_two_keywords(path)

    with openFortIO(str(path), mode=FortIO.READ_AND_WRITE_MODE) as fortio:
        fortio.truncate(offset)

    assert path.stat().st_size == offset
    assert len(ResdataFile(str(path))) == 1


def test_that_seeking_to_a_negative_position_fails(tmp_path):
    path = tmp_path / "file"
    _write_two_keywords(path)

    with openFortIO(str(path), mode=FortIO.READ_AND_WRITE_MODE) as fortio:
        assert not fortio.seek(-1)


@pytest.mark.skipif(
    not hasattr(os, "getuid") or os.getuid() == 0,
    reason="file permission semantics are unavailable or running as root",
)
def test_that_opening_an_unreadable_file_raises(tmp_path):
    path = tmp_path / "file"
    _write_two_keywords(path)
    path.chmod(0)

    try:
        with pytest.raises(OSError, match="Failed to open FortIO file"):
            FortIO(str(path), FortIO.READ_MODE)
    finally:
        path.chmod(0o644)


def test_that_reading_a_record_without_a_trailer_fails(tmp_path):
    path = tmp_path / "file"
    _write_two_keywords(path)
    path.write_bytes(path.read_bytes()[:-4])

    with openFortIO(str(path)) as fortio:
        ResdataKW.fread(fortio)
        with pytest.raises(ValueError, match="Failed to create ResdataKW instance"):
            ResdataKW.fread(fortio)


def _record_size(data_size):
    # keyword header record: 4 + (8 char name + 4 byte count + 4 char type) + 4
    # data record:           4 + data + 4
    return 24 + 2 * 4 + data_size


def _write_kw(path, mode, name, length):
    kw = ResdataKW(name, length, ResDataType.RD_INT)
    for i in range(length):
        kw[i] = i
    with openFortIO(str(path), mode=mode) as f:
        kw.fwrite(f)
    return kw


def test_that_ftell_is_zero_after_opening_a_new_file_for_write(tmp_path):
    path = tmp_path / "file"
    with openFortIO(str(path), mode=FortIO.WRITE_MODE) as f:
        assert f.get_position() == 0


def test_that_ftell_tracks_position_after_writes(tmp_path):
    path = tmp_path / "file"
    kw1 = ResdataKW("KW1", 10, ResDataType.RD_INT)
    kw2 = ResdataKW("KW2", 5, ResDataType.RD_INT)
    with openFortIO(str(path), mode=FortIO.WRITE_MODE) as f:
        kw1.fwrite(f)
        assert f.get_position() == _record_size(kw1.data_type.element_size * 10)

        pos_before_kw2 = f.get_position()
        kw2.fwrite(f)
        assert f.get_position() == pos_before_kw2 + _record_size(
            kw2.data_type.element_size * 5
        )


def test_that_ftell_is_at_end_of_file_after_opening_in_append_mode(tmp_path):
    path = tmp_path / "file"
    _write_kw(path, FortIO.WRITE_MODE, "KW1", 10)
    expected_size = os.path.getsize(path)

    with openFortIO(str(path), mode=FortIO.APPEND_MODE) as f:
        assert f.get_position() == expected_size

        kw2 = ResdataKW("KW2", 3, ResDataType.RD_INT)
        kw2.fwrite(f)
        assert f.get_position() == expected_size + _record_size(
            kw2.data_type.element_size * 3
        )


def test_that_ftell_tracks_position_after_reads(tmp_path):
    path = tmp_path / "file"
    kw1 = _write_kw(path, FortIO.WRITE_MODE, "KW1", 10)
    kw1_record_size = _record_size(kw1.data_type.element_size * 10)

    with openFortIO(str(path), mode=FortIO.READ_MODE) as f:
        assert f.get_position() == 0
        ResdataKW.fread(f)
        assert f.get_position() == kw1_record_size


def test_that_seek_and_ftell_are_consistent_in_read_mode(tmp_path):
    path = tmp_path / "file"
    _write_kw(path, FortIO.WRITE_MODE, "KW1", 10)
    file_size = os.path.getsize(path)

    with openFortIO(str(path), mode=FortIO.READ_MODE) as f:
        assert f.seek(0, whence=2)  # SEEK_END
        assert f.get_position() == file_size

        assert f.seek(0, whence=0)  # SEEK_SET
        assert f.get_position() == 0


def test_that_seek_and_ftell_are_consistent_in_write_mode(tmp_path):
    path = tmp_path / "file"
    kw1 = ResdataKW("KW1", 10, ResDataType.RD_INT)
    with openFortIO(str(path), mode=FortIO.WRITE_MODE) as f:
        kw1.fwrite(f)
        end_of_kw1 = f.get_position()

        assert f.seek(0, whence=0)  # SEEK_SET
        assert f.get_position() == 0

        assert f.seek(end_of_kw1, whence=0)  # SEEK_SET
        assert f.get_position() == end_of_kw1


def test_that_seek_and_ftell_are_consistent_in_read_and_write_mode(tmp_path):
    path = tmp_path / "file"
    open(path, "wb").close()

    kw1 = ResdataKW("KW1", 10, ResDataType.RD_INT)
    with openFortIO(str(path), mode=FortIO.READ_AND_WRITE_MODE) as f:
        assert f.get_position() == 0
        kw1.fwrite(f)
        end_of_kw1 = f.get_position()

        assert f.seek(0, whence=0)  # SEEK_SET
        assert f.get_position() == 0

        kw1_read_back = ResdataKW.fread(f)
        assert f.get_position() == end_of_kw1
        assert kw1_read_back == kw1
