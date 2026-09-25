import os
import random
import tempfile
import warnings

import hypothesis.strategies as st
import numpy as np
import numpy.testing as npt
import pytest
import resfo
from hypothesis import assume, settings
from hypothesis.extra.lark import from_lark
from hypothesis.extra.numpy import arrays, from_dtype
from hypothesis.stateful import Bundle, RuleBasedStateMachine, rule
from lark import Lark
from resdata import FileMode, ResDataType
from resdata.grid import GridGenerator, ResdataRegion
from resdata.resfile import FortIO, ResdataFile, ResdataKW, openFortIO

from tests import ResdataTest


def copy_long():
    src = ResdataKW("NAME", 100, ResDataType.RD_FLOAT)
    copy = src.sub_copy(0, 2000)


def copy_offset():
    src = ResdataKW("NAME", 100, ResDataType.RD_FLOAT)
    copy = src.sub_copy(200, 100)


class KWTest(ResdataTest):
    def test_name(self):
        kw = ResdataKW("TEST", 3, ResDataType.RD_INT)
        self.assertEqual(kw.name, "TEST")
        self.assertIn("TEST", repr(kw))
        kw.name = "SCHMEST"
        self.assertEqual(kw.name, "SCHMEST")
        self.assertIn("SCHMEST", repr(kw))

    def test_min_max(self):
        kw = ResdataKW("TEST", 3, ResDataType.RD_INT)
        kw[0] = 10
        kw[1] = 5
        kw[2] = 0

        self.assertEqual(10, kw.get_max())
        self.assertEqual(0, kw.get_min())
        self.assertEqual((0, 10), kw.get_min_max())

    def kw_test(self, data_type, data, fmt):
        name1 = "file1.txt"
        name2 = "file2.txt"
        kw = ResdataKW("TEST", len(data), data_type)
        for i, d in enumerate(data):
            kw[i] = d

        with open(name1, "w") as file1:
            kw.fprintf_data(file1, fmt)

        with open(name2, "w") as file2:
            for d in data:
                file2.write(fmt % d)
        self.assertFilesAreEqual(name1, name2)
        self.assertEqual(kw.data_type, data_type)

    def test_create(self):
        with self.assertRaises(ValueError):
            ResdataKW("ThisIsTooLong", 100, ResDataType.RD_CHAR)

    def test_header_and_str(self):
        kw = ResdataKW("TESTKW", 3, ResDataType.RD_INT)
        kw[0] = 1
        kw[1] = 2
        kw[2] = 3

        name, length, type_name = kw.header
        self.assertEqual(name, "TESTKW")
        self.assertEqual(length, 3)
        self.assertEqual(type_name, kw.type_name())

        s = kw.str(width=2, max_lines=10)
        self.assertIn("TESTKW", s)

    def test_sum(self):
        for rd_type in [ResDataType.RD_CHAR, ResDataType.RD_STRING(42)]:
            kw_string = ResdataKW("STRING", 100, rd_type)
            with self.assertRaises(ValueError):
                kw_string.sum()

        kw_int = ResdataKW("INT", 4, ResDataType.RD_INT)
        self.assertEqual(kw_int.type_name(), "INTE")
        kw_int[0] = 1
        kw_int[1] = 2
        kw_int[2] = 3
        kw_int[3] = 4
        self.assertEqual(kw_int.sum(), 10)

        kw_d = ResdataKW("D", 4, ResDataType.RD_DOUBLE)
        self.assertEqual(kw_d.type_name(), "DOUB")
        kw_d[0] = 1
        kw_d[1] = 2
        kw_d[2] = 3
        kw_d[3] = 4
        self.assertEqual(kw_d.sum(), 10)

        kw_f = ResdataKW("F", 4, ResDataType.RD_FLOAT)
        self.assertEqual(kw_f.type_name(), "REAL")
        kw_f[0] = 1
        kw_f[1] = 2
        kw_f[2] = 3
        kw_f[3] = 4
        self.assertEqual(kw_f.sum(), 10)

        kw_b = ResdataKW("F", 4, ResDataType.RD_BOOL)
        self.assertEqual(kw_b.type_name(), "LOGI")
        kw_b[0] = False
        kw_b[1] = True
        kw_b[2] = False
        kw_b[3] = True
        self.assertEqual(kw_b.sum(), 2)

    def test_fprintf(self):
        tmpdir = self.tmp_path_factory.mktemp("python.rd_kw", numbered=True)
        with self.monkeypatch.context() as mp:
            mp.chdir(tmpdir)
            self.kw_test(ResDataType.RD_INT, [0, 1, 2, 3, 4, 5], "%4d\n")
            self.kw_test(
                ResDataType.RD_FLOAT, [0.0, 1.1, 2.2, 3.3, 4.4, 5.5], "%12.6f\n"
            )
            self.kw_test(
                ResDataType.RD_DOUBLE, [0.0, 1.1, 2.2, 3.3, 4.4, 5.5], "%12.6f\n"
            )
            self.kw_test(ResDataType.RD_BOOL, [True, True, True, False, True], "%4d\n")
            self.kw_test(
                ResDataType.RD_CHAR,
                ["1", "22", "4444", "666666", "88888888"],
                "%-8s\n",
            )

            for str_len in range(1000):
                self.kw_test(
                    ResDataType.RD_STRING(str_len),
                    [str(i) * str_len for i in range(10)],
                    "%s\n",
                )

    def test_kw_write(self):
        tmpdir = self.tmp_path_factory.mktemp("python_rd_kw_writing", numbered=True)
        with self.monkeypatch.context() as mp:
            mp.chdir(tmpdir)
            data = [random.random() for i in range(10000)]

            kw = ResdataKW("TEST", len(data), ResDataType.RD_DOUBLE)
            i = 0
            for d in data:
                kw[i] = d
                i += 1

            pfx = "ResdataKW("
            self.assertEqual(pfx, repr(kw)[: len(pfx)])

            fortio = FortIO("RD_KW_TEST", FortIO.WRITE_MODE)
            kw.fwrite(fortio)
            fortio.close()

            fortio = FortIO("RD_KW_TEST")

            kw2 = ResdataKW.fread(fortio)

            self.assertTrue(kw.equal(kw2))

            rd_file = ResdataFile("RD_KW_TEST", flags=FileMode.WRITABLE)
            kw3 = rd_file["TEST"][0]
            self.assertTrue(kw.equal(kw3))
            rd_file.save_kw(kw3)
            rd_file.close()

            fortio = FortIO("RD_KW_TEST", FortIO.READ_AND_WRITE_MODE)
            kw4 = ResdataKW.fread(fortio)
            self.assertTrue(kw.equal(kw4))
            fortio.seek(0)
            kw4.fwrite(fortio)
            fortio.close()

            rd_file = ResdataFile("RD_KW_TEST")
            kw5 = rd_file["TEST"][0]
            self.assertTrue(kw.equal(kw5))

    def test_fprintf_data(self):
        tmpdir = self.tmp_path_factory.mktemp("kw_no_header", numbered=True)
        with self.monkeypatch.context() as mp:
            mp.chdir(tmpdir)
            kw = ResdataKW("REGIONS", 10, ResDataType.RD_INT)
            for i in range(len(kw)):
                kw[i] = i

            with open("test", "w") as fileH:
                kw.fprintf_data(fileH)

            data = []
            with open("test") as fileH:
                for line in fileH:
                    tmp = line.split()
                    for elm in tmp:
                        data.append(int(elm))

            for v1, v2 in zip(data, kw):
                self.assertEqual(v1, v2)

    def test_sliced_set(self):
        kw = ResdataKW("REGIONS", 10, ResDataType.RD_INT)
        kw.assign(99)
        kw[0:5] = 66
        self.assertEqual(kw[0], 66)
        self.assertEqual(kw[4], 66)
        self.assertEqual(kw[5], 99)

    def test_long_name(self):
        with self.assertRaises(ValueError):
            ResdataKW("LONGLONGNAME", 10, ResDataType.RD_INT)

        kw = ResdataKW("REGIONS", 10, ResDataType.RD_INT)
        with self.assertRaises(ValueError):
            kw.name = "LONGLONGNAME"

    def test_abs(self):
        for rd_type in [
            ResDataType.RD_CHAR,
            ResDataType.RD_BOOL,
            ResDataType.RD_STRING(32),
        ]:
            kw = ResdataKW("NAME", 10, rd_type)
            with self.assertRaises(TypeError):
                abs_kw = abs(kw)

        kw = ResdataKW("NAME", 10, ResDataType.RD_INT)
        for i in range(len(kw)):
            kw[i] = -i

        abs_kw = abs(kw)
        for i in range(len(kw)):
            self.assertEqual(kw[i], -i)
            self.assertEqual(abs_kw[i], i)

    def test_fmt(self):
        kw1 = ResdataKW("NAME1", 100, ResDataType.RD_INT)
        kw2 = ResdataKW("NAME2", 100, ResDataType.RD_INT)

        for i in range(len(kw1)):
            kw1[i] = i + 1
            kw2[i] = len(kw1) - kw1[i]

        ta = self.tmp_path_factory.mktemp("rd_kw_fmt", numbered=True)
        with self.monkeypatch.context() as mp:
            mp.chdir(ta)
            with openFortIO("TEST.FINIT", FortIO.WRITE_MODE, fmt_file=True) as f:
                kw1.fwrite(f)
                kw2.fwrite(f)

            with openFortIO("TEST.FINIT", fmt_file=True) as f:
                kw1b = ResdataKW.fread(f)
                kw2b = ResdataKW.fread(f)

            self.assertTrue(kw1 == kw1b)
            self.assertTrue(kw2 == kw2b)

            f = ResdataFile("TEST.FINIT")
            self.assertTrue(kw1 == f[0])
            self.assertTrue(kw2 == f[1])

    def test_first_different(self):
        kw1 = ResdataKW("NAME1", 100, ResDataType.RD_INT)
        kw2 = ResdataKW("NAME2", 100, ResDataType.RD_INT)
        kw3 = ResdataKW("NAME2", 200, ResDataType.RD_INT)
        kw4 = ResdataKW("NAME2", 100, ResDataType.RD_FLOAT)
        kw5 = ResdataKW("NAME2", 100, ResDataType.RD_FLOAT)

        with self.assertRaises(IndexError):
            ResdataKW.first_different(kw1, kw2, offset=100)

        with self.assertRaises(ValueError):
            ResdataKW.first_different(kw1, kw3)

        with self.assertRaises(TypeError):
            ResdataKW.first_different(kw1, kw4)

        with self.assertRaises(IndexError):
            kw1.first_different(kw2, offset=100)

        with self.assertRaises(ValueError):
            kw1.first_different(kw3)

        with self.assertRaises(TypeError):
            kw1.first_different(kw4)

        kw1.assign(1)
        kw2.assign(1)

        self.assertEqual(kw1.first_different(kw2), len(kw1))

        kw1[0] = 100
        self.assertEqual(kw1.first_different(kw2), 0)
        self.assertEqual(kw1.first_different(kw2, offset=1), len(kw1))
        kw1[10] = 100
        self.assertEqual(kw1.first_different(kw2, offset=1), 10)

        kw4.assign(1.0)
        kw5.assign(1.0)
        self.assertEqual(kw4.first_different(kw5), len(kw4))

        kw4[10] *= 1.0001
        self.assertEqual(kw4.first_different(kw5), 10)

        self.assertEqual(kw4.first_different(kw5, epsilon=1.0), len(kw4))
        self.assertEqual(kw4.first_different(kw5, epsilon=0.0000001), 10)

    def test_numeric_equal(self):
        kw1 = ResdataKW("Name1", 10, ResDataType.RD_DOUBLE)
        kw2 = ResdataKW("Name1", 10, ResDataType.RD_DOUBLE)

        shift = 0.0001
        value = 1000

        abs_diff = shift
        rel_diff = shift / (shift + 2 * value)
        kw1.assign(value)
        kw2.assign(value + shift)

        self.assertTrue(
            kw1.equal_numeric(
                kw2, abs_epsilon=abs_diff * 1.1, rel_epsilon=rel_diff * 1.1
            )
        )
        self.assertFalse(
            kw1.equal_numeric(
                kw2, abs_epsilon=abs_diff * 1.1, rel_epsilon=rel_diff * 0.9
            )
        )
        self.assertFalse(
            kw1.equal_numeric(
                kw2, abs_epsilon=abs_diff * 0.9, rel_epsilon=rel_diff * 1.1
            )
        )
        self.assertTrue(
            kw1.equal_numeric(kw2, abs_epsilon=0, rel_epsilon=rel_diff * 1.1)
        )
        self.assertTrue(
            kw1.equal_numeric(kw2, abs_epsilon=abs_diff * 1.1, rel_epsilon=0)
        )

    def test_mul(self):
        kw1 = ResdataKW("Name1", 10, ResDataType.RD_INT)
        kw1.assign(10)

        kw2 = ResdataKW("Name1", 10, ResDataType.RD_INT)
        kw2.assign(2)

        kw3 = kw1 * kw2
        kw4 = kw1 + kw2
        self.assertEqual(len(kw3), len(kw1))
        self.assertEqual(len(kw4), len(kw1))
        for v in kw3:
            self.assertEqual(v, 20)

        for v in kw4:
            self.assertEqual(v, 12)

        kw2 *= 2
        for v in kw2:
            self.assertEqual(v, 4)

    def test_div(self):
        kw1 = ResdataKW("Name1", 10, ResDataType.RD_DOUBLE)
        kw1.assign(4.0)
        kw2 = ResdataKW("Name1", 10, ResDataType.RD_DOUBLE)
        kw2.assign(kw1)

        kw1.__idiv__(kw1)
        for v in kw1:
            self.assertEqual(v, 1.0)
        self.assertNotEqual(kw1, kw2)

    def test_numpy(self):
        kw1 = ResdataKW("DOUBLE", 10, ResDataType.RD_DOUBLE)

        view = kw1.numpy_view()
        copy = kw1.numpy_copy()
        kw2 = kw1.sub_copy(1, 3)
        self.assertEqual(len(kw2), 3)

        self.assertTrue(copy[0] == kw1[0])
        self.assertTrue(view[0] == kw1[0])

        kw1[0] += 1
        self.assertTrue(view[0] == kw1[0])
        self.assertTrue(copy[0] == kw1[0] - 1)

        for rd_type in [
            ResDataType.RD_CHAR,
            ResDataType.RD_STRING(19),
        ]:
            kw2 = ResdataKW("TEST_KW", 10, rd_type)
            with self.assertRaises(ValueError):
                kw2.numpy_view()

        kw3 = ResdataKW("BOOL_KW", 10, ResDataType.RD_BOOL)
        for i in range(len(kw3)):
            kw3[i] = i % 2 == 0

        bool_view = kw3.numpy_view()
        bool_copy = kw3.numpy_copy()
        self.assertTrue(np.array_equal(bool_view, bool_copy))
        for i in range(len(kw3)):
            self.assertEqual(bool(bool_view[i]), kw3[i])

        kw3[0] = not kw3[0]
        self.assertEqual(bool(bool_view[0]), kw3[0])
        self.assertNotEqual(bool_copy[0], kw3[0])

    def test_slice(self):
        N = 100
        kw = ResdataKW("KW", N, ResDataType.RD_INT)
        for i in range(len(kw)):
            kw[i] = i

        even = kw[0 : len(kw) : 2]
        odd = kw[1 : len(kw) : 2]

        self.assertEqual(len(even), N / 2)
        self.assertEqual(len(odd), N / 2)

        for i in range(len(even)):
            self.assertEqual(even[i], 2 * i)
            self.assertEqual(odd[i], 2 * i + 1)

    def test_resize(self):
        N = 4
        kw = ResdataKW("KW", N, ResDataType.RD_INT)
        for i in range(N):
            kw[i] = i

        kw.resize(2 * N)
        self.assertEqual(len(kw), 2 * N)
        for i in range(N):
            self.assertEqual(kw[i], i)

        kw.resize(N / 2)
        self.assertEqual(len(kw), N / 2)
        for i in range(int(N / 2)):
            self.assertEqual(kw[i], i)

    def test_typename(self):
        kw = ResdataKW("KW", 100, ResDataType.RD_INT)

        self.assertEqual(kw.type_name(), "INTE")

    def test_string_alloc(self):
        kw = ResdataKW("KW", 10, ResDataType.RD_STRING(30))

        for i in range(10):
            kw[i] = str(i) * 30

        for i in range(10):
            self.assertEqual(str(i) * 30, kw[i])

    def test_string_write_read_unformatted(self):
        for str_len in range(1000):
            tmpdir = self.tmp_path_factory.mktemp("my_space", numbered=True)
            with self.monkeypatch.context() as mp:
                mp.chdir(tmpdir)
                kw = ResdataKW("TEST_KW", 10, ResDataType.RD_STRING(str_len))
                for i in range(10):
                    kw[i] = str(i) * str_len

                file_name = "rd_kw_test"

                with openFortIO(file_name, mode=FortIO.WRITE_MODE) as fortio:
                    kw.fwrite(fortio)

                with openFortIO(file_name) as fortio:
                    loaded_kw = ResdataKW.fread(fortio)

                self.assertEqual(kw, loaded_kw)

    def test_string_write_read_formatted(self):
        for str_len in range(1000):
            tmpdir = self.tmp_path_factory.mktemp("my_space", numbered=True)
            with self.monkeypatch.context() as mp:
                mp.chdir(tmpdir)
                kw = ResdataKW("TEST_KW", 10, ResDataType.RD_STRING(str_len))
                for i in range(10):
                    kw[i] = str(i) * str_len

                file_name = "rd_kw_test"
                with openFortIO(
                    file_name, mode=FortIO.WRITE_MODE, fmt_file=True
                ) as fortio:
                    kw.fwrite(fortio)

                with openFortIO(file_name, fmt_file=True) as fortio:
                    loaded_kw = ResdataKW.fread(fortio)

                self.assertEqual(kw, loaded_kw)

    def test_string_padding(self):
        kw = ResdataKW("TEST_KW", 1, ResDataType.RD_STRING(4))
        kw[0] = "AB"
        self.assertEqual(kw[0], "AB  ")

        kw = ResdataKW("TEST_KW", 1, ResDataType.RD_CHAR)
        kw[0] = "ABCD"
        self.assertEqual(kw[0], "ABCD    ")

    def test_add_squared(self):
        kw1 = ResdataKW("TEST_KW", 3, ResDataType.RD_STRING(4))
        kw2 = ResdataKW("TEST_KW", 3, ResDataType.RD_STRING(4))

        with self.assertRaises(TypeError):
            kw1.add_squared(kw2)

        kw1 = ResdataKW("T1", 10, ResDataType.RD_INT)
        kw2 = ResdataKW("T2", 11, ResDataType.RD_INT)
        with self.assertRaises(ValueError):
            kw1.add_squared(kw2)

        kw2 = ResdataKW("T", 10, ResDataType.RD_FLOAT)
        with self.assertRaises(ValueError):
            kw1.add_squared(kw2)

        kw2 = ResdataKW("T2", 10, ResDataType.RD_INT)
        kw2.assign(2)
        kw1.add_squared(kw2)

        for elm in kw1:
            self.assertEqual(elm, 4)

    def test_scatter_copy(self):
        source = ResdataKW("SOURCE", 4, ResDataType.RD_INT)
        with self.assertRaises(TypeError):
            copy = source.scatter_copy([1, 1, 1, 1])

        actnum = ResdataKW("ACTNUM", 6, ResDataType.RD_FLOAT)
        with self.assertRaises(ValueError):
            copy = source.scatter_copy(actnum)

        actnum = ResdataKW("ACTNUM", 8, ResDataType.RD_INT)
        actnum[0] = 1
        actnum[1] = 1
        with self.assertRaises(ValueError):
            copy = source.scatter_copy(actnum)

        actnum.assign(1)
        with self.assertRaises(ValueError):
            copy = source.scatter_copy(actnum)

        for i in range(4):
            source[i] = i + 1
            actnum[2 * i] = 0

        # src = [1,2,3,4]
        # actnum = [0,1,0,1,0,1,0,1]
        # copy = [0,1,0,2,0,3,0,4]
        copy = source.scatter_copy(actnum)
        for i in range(4):
            self.assertEqual(copy[2 * i + 1], i + 1)

    def test_safe_div(self):
        kw1 = ResdataKW("SOURCE", 10, ResDataType.RD_INT)
        kw2 = ResdataKW("XXX", 11, ResDataType.RD_INT)

        with self.assertRaises(ValueError):
            kw1.safe_div(kw2)

        kw1 = ResdataKW("SOURCE", 2, ResDataType.RD_FLOAT)
        kw1.assign(10)

        kw2 = ResdataKW("DIV", 2, ResDataType.RD_INT)
        kw2[0] = 0
        kw2[1] = 2

        kw1.safe_div(kw2)
        self.assertEqual(kw1[0], 10)
        self.assertEqual(kw1[1], 5)

    def test_fmu_stat_workflow(self):
        N = 100
        global_size = 100
        active_size = 50
        tmpdir = self.tmp_path_factory.mktemp("FMU_FILES", numbered=True)
        with self.monkeypatch.context() as mp:
            mp.chdir(tmpdir)
            for i in range(N):
                permx = ResdataKW("PERMX", active_size, ResDataType.RD_FLOAT)
                poro = ResdataKW("PORO", active_size, ResDataType.RD_FLOAT)
                porv = ResdataKW("PORV", global_size, ResDataType.RD_FLOAT)

                porv.assign(0)
                for g in random.sample(range(global_size), active_size):
                    porv[g] = 1

                permx.assign(random.random())
                poro.assign(random.random())

                with openFortIO("TEST%d.INIT" % i, FortIO.WRITE_MODE) as f:
                    permx.fwrite(f)
                    poro.fwrite(f)
                    porv.fwrite(f)

            mean_permx = ResdataKW("PERMX", global_size, ResDataType.RD_FLOAT)
            std_permx = ResdataKW("PERMX", global_size, ResDataType.RD_FLOAT)
            mean_poro = ResdataKW("PORO", global_size, ResDataType.RD_FLOAT)
            std_poro = ResdataKW("PORO", global_size, ResDataType.RD_FLOAT)

            count = ResdataKW("COUNT", global_size, ResDataType.RD_INT)
            for i in range(N):
                f = ResdataFile("TEST%d.INIT" % i)

                porv = f["PORV"][0]
                permx = f["PERMX"][0]
                poro = f["PORO"][0]

                actnum = porv.create_actnum()

                global_permx = permx.scatter_copy(actnum)
                mean_permx += global_permx
                std_permx.add_squared(global_permx)

                global_poro = poro.scatter_copy(actnum)
                mean_poro += global_poro
                std_poro.add_squared(global_poro)

                count += actnum

            mean_permx.safe_div(count)
            std_permx.safe_div(count)
            std_permx -= mean_permx * mean_permx
            std_permx.isqrt()

            mean_poro.safe_div(count)
            std_poro.safe_div(count)
            std_poro -= mean_poro * mean_poro
            std_poro.isqrt()


def test_iadd():
    kw1 = ResdataKW("KW1", 10, ResDataType.RD_INT)
    kw2 = ResdataKW("KW2", 10, ResDataType.RD_INT)
    for i in range(len(kw2)):
        kw2[i] = 1

    assert hash(kw1) != hash(kw2)

    kw1 += kw2

    assert list(kw1) == list(kw2)

    with pytest.raises(TypeError, match="Type mismatch"):
        kw1 += "a"


def test_imul():
    kw1 = ResdataKW("KW1", 5, ResDataType.RD_INT)
    for i in range(len(kw1)):
        kw1[i] = 1
    kw1 *= 10

    assert list(kw1) == [10] * 5

    with pytest.raises(TypeError, match="Type mismatch"):
        kw1 *= 3.2

    kw2 = ResdataKW("KW2", 5, ResDataType.RD_FLOAT)
    with pytest.raises(TypeError, match="Only muliplication with scalar supported"):
        kw2 *= "a"


def test_assign():
    kw1 = ResdataKW("KW1", 5, ResDataType.RD_INT)
    kw2 = ResdataKW("KW2", 6, ResDataType.RD_INT)
    kw3 = ResdataKW("KW3", 5, ResDataType.RD_FLOAT)
    for i in range(len(kw1)):
        kw1[i] = 1
    with pytest.raises(TypeError, match="Type / size mismatch"):
        kw2.assign(kw1)
    with pytest.raises(TypeError, match="Type / size mismatch"):
        kw3.assign(kw1)
    with pytest.raises(TypeError, match="Type mismatch"):
        kw2.assign("a")
    with pytest.raises(TypeError, match="Only muliplication with scalar supported"):
        kw3.assign("a")


def test_apply():
    kw1 = ResdataKW("KW1", 5, ResDataType.RD_INT)
    kw2 = ResdataKW("KW2", 6, ResDataType.RD_INT)
    kw3 = ResdataKW("KW3", 5, ResDataType.RD_FLOAT)
    kw1.assign(1)
    kw1.apply(lambda x: x + 1)
    assert list(kw1) == [2] * 5
    kw2.assign(5)
    kw2.apply(lambda x, y: x + y, arg=5)
    assert list(kw2) == [10] * 6
    grid = GridGenerator.create_rectangular(dims=(5, 1, 1), dV=(1, 1, 1))
    region = ResdataRegion(grid, True)
    kw3.assign(3.0)
    kw3.apply(lambda x: x + 1.0, mask=region)
    assert list(kw3) == [4.0] * 5


def _write_grdecl(tmp_path, name, body):
    path = tmp_path / name
    path.write_text(body)
    return path


def test_missing_keyword_raises(tmp_path):
    path = _write_grdecl(tmp_path, "empty.grdecl", "PORO\n/\n")
    with pytest.raises(ValueError), open(str(path)) as fh:
        _ = ResdataKW.read_grdecl(fh, "NOTPORO", rd_type=ResDataType.RD_FLOAT)


def test_read_grdecl_empty_body_returns_zero_len_kw(tmp_path):
    path = _write_grdecl(tmp_path, "empty.grdecl", "PORO\n/\n")
    with open(str(path)) as fh:
        kw = ResdataKW.read_grdecl(fh, "PORO", rd_type=ResDataType.RD_FLOAT)
    assert kw is not None
    assert kw.name == "PORO"
    assert len(kw) == 0


def test_read_grdecl_kw_none_loads_first_keyword(tmp_path):
    path = _write_grdecl(tmp_path, "first.grdecl", "PORO\n  0.1 0.2 0.3 /\n")
    with open(str(path)) as fh:
        kw = ResdataKW.read_grdecl(fh, None, rd_type=ResDataType.RD_FLOAT)
    assert kw is not None
    assert kw.name == "PORO"
    assert len(kw) == 3


def test_read_grdecl_int_strict_raises_on_malformed(tmp_path):
    path = _write_grdecl(tmp_path, "intkw.grdecl", "INTKW\n  1 2 FOO 3 /\n")
    with open(str(path)) as fh:
        with pytest.raises(
            ValueError,
            match=r'Malformed content:"FOO" when reading keyword:INTKW',
        ):
            ResdataKW.read_grdecl(fh, "INTKW", rd_type=ResDataType.RD_INT)


def test_read_grdecl_float_strict_raises_on_malformed(tmp_path):
    path = _write_grdecl(tmp_path, "fltkw.grdecl", "FLTKW\n  1.0 2.0 BAR 3.0 /\n")
    with open(str(path)) as fh:
        with pytest.raises(
            ValueError,
            match=r'Malformed content:"BAR" when reading keyword:FLTKW',
        ):
            ResdataKW.read_grdecl(fh, "FLTKW", rd_type=ResDataType.RD_FLOAT)


def read_kw_from_bytes(tmp_path, kw_bytes: bytes) -> ResdataKW:
    filepath = tmp_path / "test.bin"
    filepath.write_bytes(kw_bytes)
    # Avoid using the openFortIO context manager to get around
    # https://github.com/equinor/resdata/issues/1186
    fortio = None
    try:
        fortio = FortIO(str(filepath), fmt_file=False, endian_flip_header=True)
        kw = ResdataKW.fread(fortio)
    finally:
        if fortio is not None:
            fortio.close()
    return kw


def test_that_single_element_keyword_can_be_read(tmp_path):
    kw = read_kw_from_bytes(
        tmp_path,
        b"\x00\x00\x00\x10KEYWORD1\x00\x00\x00\x01INTE\x00\x00\x00\x10"
        b"\x00\x00\x00\x04\x00\x00\x00\x01\x00\x00\x00\x04",
    )
    assert len(kw) == 1
    assert kw.name == "KEYWORD1"
    assert kw[0] == 1


def test_that_zero_sized_keywords_can_be_read(tmp_path):
    kw = read_kw_from_bytes(
        tmp_path, b"\x00\x00\x00\x10KEYWORD1\x00\x00\x00\x00INTE\x00\x00\x00\x10"
    )
    assert len(kw) == 0
    assert kw.name == "KEYWORD1"


def test_that_short_data_section_raises_value_error(tmp_path):
    with pytest.raises(ValueError, match=r"Failed to create ResdataKW instance"):
        _ = read_kw_from_bytes(
            tmp_path, b"\x00\x00\x00\x10KEYWORD1\x00\x00\x00\x01INTE\x00\x00\x00\x10"
        )


def test_that_oversized_record_size_raises_value_error(tmp_path):
    with pytest.raises(ValueError, match=r"Failed to create ResdataKW instance"):
        _ = read_kw_from_bytes(
            tmp_path,
            b"\x00\x00\x00\x10KEYWORD1\x00\x00\x00\x01INTE\x00\x00\x00\x10"
            b"\x00\x00\x00\x0f" + b"\x00" * 1000,
        )


def test_that_negative_record_size_raises_value_error(tmp_path):
    with pytest.raises(ValueError, match=r"Failed to create ResdataKW instance"):
        _ = read_kw_from_bytes(
            tmp_path,
            b"\x00\x00\x00\x10KEYWORD1\x00\x00\x00\x01INTE\x00\x00\x00\x10\xf0\x00\x00\x00",
        )


def test_that_mismatch_in_end_record_raises_value_error(tmp_path):
    with pytest.raises(ValueError, match=r"Failed to create ResdataKW instance"):
        _ = read_kw_from_bytes(
            tmp_path,
            b"\x00\x00\x00\x10KEYWORD1\x00\x00\x00\x01INTE\x00\x00\x00\x10"
            b"\x00\x00\x00\x04\x00\x00\x00\x00\x00\x00\x00\x05",
        )


def test_that_unformatted_rd_files_are_read_in_blocks(tmp_path):
    kw = read_kw_from_bytes(
        tmp_path,
        b"\x00\x00\x00\x10KEYWORD1"
        + (2300).to_bytes(4, byteorder="big")
        + b"INTE\x00\x00\x00\x10"
        + (
            (4000).to_bytes(4, byteorder="big")
            + b"\x00\x00\x00\x01" * 1000
            + (4000).to_bytes(4, byteorder="big")
        )
        * 2
        + (
            (4 * 300).to_bytes(4, byteorder="big")
            + b"\x00\x00\x00\x01" * 300
            + (4 * 300).to_bytes(4, byteorder="big")
        ),
    )
    assert len(kw) == 2300
    assert kw.name == "KEYWORD1"


def test_that_non_zero_kw_can_be_read_after_zero_kw(tmp_path):
    filepath = tmp_path / "test.bin"
    filepath.write_bytes(
        b"\x00\x00\x00\x10KEYWORD1\x00\x00\x00\x00INTE\x00\x00\x00\x10"
        b"\x00\x00\x00\x10KEYWORD2\x00\x00\x00\x01INTE\x00\x00\x00\x10"
        b"\x00\x00\x00\x04\x00\x00\x00\x01\x00\x00\x00\x04",
    )
    # Avoid using the openFortIO context manager to get around
    # https://github.com/equinor/resdata/issues/1186
    fortio = None
    try:
        fortio = FortIO(str(filepath), fmt_file=False, endian_flip_header=True)
        kw1 = ResdataKW.fread(fortio)
        kw2 = ResdataKW.fread(fortio)
    finally:
        if fortio is not None:
            fortio.close()

    assert kw1.name == "KEYWORD1"
    assert len(kw1) == 0
    assert kw2.name == "KEYWORD2"
    assert len(kw2) == 1


@st.composite
def keywords(draw, size=8):
    return draw(
        st.text(
            min_size=size,
            max_size=size,
            alphabet=st.characters(min_codepoint=40, max_codepoint=126),
        )
    )


@st.composite
def str_arrays(draw):
    size = draw(st.integers(min_value=1, max_value=98))
    return draw(st.builds(np.array, st.lists(keywords(size), min_size=1))).astype(
        "|S" + str(size)
    )


array_shapes = st.integers(min_value=1, max_value=32).map(lambda n: (n,))

int_arrays = arrays(dtype=np.int32, shape=array_shapes)
float_arrays = arrays(
    dtype=np.float32,
    elements=st.floats(width=32, min_value=-1e7, max_value=1e7),
    shape=array_shapes,
)

double_arrays = arrays(
    dtype=np.float64,
    elements=st.floats(width=64, min_value=-1e10, max_value=1e10),
    shape=array_shapes,
)

numeric_arrays = st.one_of(float_arrays, double_arrays, int_arrays)
numeric_res_datas = st.tuples(keywords(8), numeric_arrays)
str_res_datas = st.tuples(keywords(8), str_arrays())


def write_with_resfo_and_read_with_resdata(res_data, file_format):
    is_formatted = file_format == resfo.Format.FORMATTED
    mode = "w+" if is_formatted else "w+b"
    kw = None
    with tempfile.NamedTemporaryFile(mode=mode, delete=False) as namedtmp:
        resfo.write(namedtmp, [res_data], file_format)
        namedtmp.flush()

        assert FortIO.is_fortran_file(namedtmp.name) != is_formatted

        with openFortIO(namedtmp.name, fmt_file=is_formatted) as f:
            kw = ResdataKW.fread(f)

    assert kw is not None
    assert kw.name == res_data[0]
    return kw


def read_grdecl_from_text(rd_type, text):
    fname = None
    try:
        with tempfile.NamedTemporaryFile("w", suffix=".grdecl", delete=False) as f:
            f.write(text)
            fname = f.name

        with open(f.name) as fh:
            kw = ResdataKW.read_grdecl(fh, None, rd_type=rd_type)
    finally:
        if fname:
            os.unlink(fname)

    return kw


def nice_from_dtype(dtype):
    return from_dtype(dtype, allow_infinity=False, allow_nan=False)


GRDECL_INT_GRAMMAR = Lark(r"""
start: header tokens "/" _WS

header: KW_NAME _WS

tokens: (_token _WS)*

_token: integer | multiplier | comment

integer: INT
multiplier: POS_INT "*" INT
comment: COMMENT

KW_NAME: /[A-Z][A-Z0-9]{0,7}/
INT: /-?[0-9]{1,4}/
POS_INT: /[1-9][0-9]{0,2}/
COMMENT: /--[ \t][ \ta-zA-Z0-9]{0,20}\n/
_WS: /[ \t\n]+/
""")

GRDECL_FLOAT_GRAMMAR = Lark(r"""
start: header tokens "/" _WS

header: KW_NAME _WS

tokens: (_token _WS)*

_token: number | multiplier | comment

number: NUM
multiplier: POS_INT "*" NUM
comment: COMMENT

KW_NAME: /[A-Z][A-Z0-9]{0,7}/
NUM: /-?[0-9]{1,3}(\.[0-9]{1,3})?([eE][+-]?[0-9]{1,2})?/
POS_INT: /[1-9][0-9]{0,2}/
COMMENT: /--[ \t][ \ta-zA-Z0-9]{0,20}\n/
_WS: /[ \t\n]+/
""")


def order_of(values):
    return np.linalg.norm(values, ord=np.inf)


@settings(max_examples=100, suppress_health_check=["filter_too_much"])
class StatefulKwTest(RuleBasedStateMachine):
    numeric_kws = Bundle("numeric_kws")
    str_kws = Bundle("str_kws")

    @staticmethod
    def draw_scalar(data, dtype):
        v = data.draw(nice_from_dtype(dtype))
        actual_v = int(v) if dtype == np.int32 else float(v)
        return actual_v, v

    @rule(
        res_data=numeric_res_datas,
        file_format=st.sampled_from(resfo.Format),
        target=numeric_kws,
    )
    def create_numeric_kw(self, res_data, file_format):
        kw = write_with_resfo_and_read_with_resdata(res_data, file_format)

        npt.assert_allclose(kw.numpy_view(), res_data[1], rtol=1e-2, atol=1e-6)

        # will cause a difference which may eventually exceed tolerance.
        # Therefore, the truncated values are assigned to the model values.
        match file_format:
            case resfo.Format.FORMATTED:
                res_data[1][:] = kw.numpy_view()

        return (kw, res_data)

    @rule(
        res_data=str_res_datas,
        file_format=st.sampled_from(resfo.Format),
        target=str_kws,
    )
    def create_str_kw(self, res_data, file_format):
        kw = write_with_resfo_and_read_with_resdata(res_data, file_format)

        assert [kw[i] for i in range(len(kw))] == [s.decode() for s in res_data[1]]

        return (kw, res_data)

    @rule(
        args=st.one_of(
            st.tuples(st.just(ResDataType.RD_INT), from_lark(GRDECL_INT_GRAMMAR)),
            st.tuples(st.just(ResDataType.RD_FLOAT), from_lark(GRDECL_FLOAT_GRAMMAR)),
            st.tuples(st.just(ResDataType.RD_DOUBLE), from_lark(GRDECL_FLOAT_GRAMMAR)),
        ),
        target=numeric_kws,
    )
    def create_kw_from_grdecl(self, args):
        try:
            kw = read_grdecl_from_text(*args)
        except Exception:
            assume(False)

        return (kw, (kw.name, kw.numpy_view().copy()))

    @rule(kw=numeric_kws)
    def getitem_numeric(self, kw):
        actual_kw, (_, model_values) = kw
        for i in range(len(model_values)):
            assert (np.isnan(actual_kw[i]) and np.isnan(model_values[i])) or (
                actual_kw[i] == pytest.approx(model_values[i], rel=1e-6, abs=1e-6)
            )

    @rule(kw=str_kws)
    def getitem_str(self, kw):
        actual_kw, (_, model_values) = kw
        for i in range(len(model_values)):
            assert actual_kw[i] == model_values[i].decode()

    @rule(kw=st.one_of(numeric_kws, str_kws))
    def len(self, kw):
        actual_kw, (_, model_values) = kw
        assert len(actual_kw) == len(model_values)

    @rule(kw=st.one_of(numeric_kws, str_kws))
    def name(self, kw):
        actual_kw, (model_name, _) = kw
        assert actual_kw.name == model_name
        assert actual_kw.get_name() == model_name

    @rule(kw=st.one_of(numeric_kws, str_kws))
    def header(self, kw):
        actual_kw, (model_name, model_values) = kw
        header = actual_kw.header
        assert header == (model_name, len(model_values), actual_kw.type_name())

    @rule(kw=numeric_kws)
    def check_data_type_numeric(self, kw):
        actual_kw, (_, model_values) = kw
        dt = actual_kw.data_type
        if model_values.dtype == np.int32:
            assert dt.is_int()
        elif model_values.dtype == np.float32:
            assert dt.is_float()
        elif model_values.dtype == np.float64:
            assert dt.is_double()
        else:
            pytest.fail("unexpected numeric dtype %s" % model_values.dtype)
        assert isinstance(actual_kw.type_name(), str)
        assert actual_kw.type_name() != ""

    @rule(kw=str_kws)
    def check_data_type_str(self, kw):
        actual_kw, _ = kw
        dt = actual_kw.data_type
        assert dt.is_char() or dt.is_string()
        assert isinstance(actual_kw.type_name(), str)
        assert actual_kw.type_name() != ""

    @rule(kw=numeric_kws)
    def is_numeric_true(self, kw):
        actual_kw, _ = kw
        assert actual_kw.is_numeric() is True

    @rule(kw=str_kws)
    def is_numeric_false(self, kw):
        actual_kw, _ = kw
        assert actual_kw.is_numeric() is False

    @rule(kw=numeric_kws)
    def numpy_view(self, kw):
        actual_kw, (_, model_values) = kw
        view = actual_kw.numpy_view()
        assert len(view) == len(model_values)
        if model_values.dtype == np.int32:
            assert (view == model_values).all()
        else:
            npt.assert_allclose(view, model_values, rtol=1e-2, atol=1e-6)

    @rule(kw=numeric_kws)
    def numpy_copy(self, kw):
        actual_kw, (_, model_values) = kw
        cp = actual_kw.numpy_copy()
        assert len(cp) == len(model_values)
        if model_values.dtype == np.int32:
            assert (cp == model_values).all()
        else:
            npt.assert_allclose(cp, model_values, rtol=1e-2, atol=1e-6)

        # mutating the copy must not change original
        if len(actual_kw):
            original_first = actual_kw[0]
            if model_values.dtype == np.int32:
                cp[0] = cp[0] + 1
            else:
                cp[0] = cp[0] + 1.0
            after_first = actual_kw[0]
            assert (np.isnan(after_first) and np.isnan(original_first)) or (
                after_first == pytest.approx(original_first, rel=1e-6, abs=1e-6)
            )

    @rule(kw=numeric_kws)
    def get_min_max(self, kw):
        actual_kw, (_, model_values) = kw
        assume(len(model_values))
        mn = actual_kw.get_min()
        mx = actual_kw.get_max()
        amn, amx = actual_kw.get_min_max()
        assert (np.isnan(amn) or np.isnan(mn)) or amn == mn
        assert (np.isnan(amx) or np.isnan(mx)) or amx == mx

        expected_min = float(model_values.min())
        expected_max = float(model_values.max())

        assert (np.isnan(mn) or np.isnan(expected_min)) or (
            mn == pytest.approx(expected_min, rel=1e-6, abs=1e-6)
        )
        assert (np.isnan(mx) or np.isnan(expected_max)) or (
            mx == pytest.approx(expected_max, rel=1e-6, abs=1e-6)
        )

    @rule(kw=numeric_kws)
    def sum(self, kw):
        actual_kw, (_, model_values) = kw

        actual_sum = actual_kw.sum()
        model_sum = model_values.sum(dtype=model_values.dtype)

        assert (np.isnan(actual_sum) and np.isnan(model_sum)) or (
            actual_sum == pytest.approx(model_sum, rel=1e-2, abs=1e-5)
        )

    @rule(kw=st.one_of(numeric_kws, str_kws))
    def equal_reflective(self, kw):
        actual_kw, _ = kw
        assert actual_kw.equal(actual_kw)
        assert actual_kw == actual_kw

        cp = actual_kw.copy()
        assert actual_kw.equal(cp)
        assert actual_kw == cp
        assert actual_kw.equal_numeric(cp)

    @rule(kw=st.one_of(numeric_kws, str_kws))
    def hash_reflective(self, kw):
        actual_kw, _ = kw
        h1 = hash(actual_kw)
        h2 = hash(actual_kw)
        assert h1 == h2

        assert hash(actual_kw.copy()) == h1

    @rule(kw=st.one_of(numeric_kws, str_kws))
    def fort_io_size(self, kw):
        actual_kw, _ = kw
        size = actual_kw.fort_io_size()
        assert isinstance(size, int)
        assert size > 0

    @rule(kw=st.one_of(numeric_kws, str_kws))
    def str(self, kw):
        actual_kw, (model_name, _) = kw
        s = str(actual_kw)
        assert isinstance(s, str)
        assert model_name in s
        s2 = actual_kw.str()
        assert isinstance(s2, str)
        assert model_name in s2

    @rule(data=st.data(), kw=st.one_of(numeric_kws, str_kws))
    def slice(self, data, kw):
        actual_kw, (_, model_values) = kw
        assume(len(model_values) >= 1)
        start = data.draw(st.integers(min_value=0, max_value=len(model_values) - 1))
        stop = data.draw(st.integers(min_value=start, max_value=len(model_values)))

        actual_sliced = actual_kw[start:stop]
        model_sliced = model_values[start:stop]

        if start == stop:
            assert actual_sliced is None
        elif model_values.dtype == np.int32:
            npt.assert_equal(actual_sliced.numpy_view(), model_sliced)
        elif not actual_kw.is_numeric():
            assert [actual_sliced[i] for i in range(len(actual_sliced))] == [
                m.decode() for m in model_sliced
            ]
        else:
            npt.assert_allclose(
                actual_sliced.numpy_view(), model_sliced, rtol=1e-2, atol=1e-6
            )

    @rule(data=st.data(), kw=numeric_kws)
    def assign(self, data, kw):
        actual_kw, (_, model_values) = kw
        av, v = self.draw_scalar(data, model_values.dtype)

        actual_kw.assign(av)
        model_values[:] = v

        npt.assert_allclose(actual_kw.numpy_view(), model_values, rtol=1e-2, atol=1e-6)

    @rule(kw=numeric_kws)
    def apply(self, kw):
        actual_kw, (_, model_values) = kw

        actual_kw.apply(lambda x: x + 1)
        model_values += 1

        npt.assert_allclose(actual_kw.numpy_view(), model_values, rtol=1e-2, atol=1e-6)

    @rule(kw=numeric_kws)
    def mut_isqrt(self, kw):
        actual_kw, (_, model_values) = kw
        # Only valid on non-negative values.
        assume(bool(np.all(model_values >= 0)))
        # Skip integers: the rounding semantics differ from numpy
        assume(model_values.dtype != np.int32)

        actual_kw.isqrt()
        model_values[:] = np.sqrt(model_values)

        npt.assert_allclose(actual_kw.numpy_view(), model_values, rtol=1e-2, atol=1e-5)

    @rule(kw1=numeric_kws, kw2=numeric_kws)
    def add_squared(self, kw1, kw2):
        akw1, (_, model_values1) = kw1
        akw2, (_, model_values2) = kw2
        assume(bool(np.all(model_values2 >= 0)))
        assume(akw1.assert_binary(akw2))

        akw1.add_squared(akw2)
        model_values1 += model_values2 * model_values2

        size_order = max(1.0, order_of(model_values2), order_of(model_values1))
        npt.assert_allclose(akw1.numpy_view(), model_values1, atol=size_order * 1e-6)
        npt.assert_allclose(akw2.numpy_view(), model_values2, atol=size_order * 1e-6)

    @rule(data=st.data(), kw=numeric_kws)
    def setitem_numeric(self, data, kw):
        actual_kw, (_, model_values) = kw
        assume(len(model_values) >= 1)
        i = data.draw(st.integers(min_value=0, max_value=len(model_values) - 1))
        av, v = self.draw_scalar(data, model_values.dtype)

        actual_kw[i] = av
        model_values[i] = v

        npt.assert_allclose(actual_kw.numpy_view(), model_values, rtol=1e-2, atol=1e-6)

    @rule(data=st.data(), kw=str_kws)
    def setitem_str(self, data, kw):
        actual_kw, (_, model_values) = kw
        assume(len(model_values) >= 1)
        i = data.draw(st.integers(min_value=0, max_value=len(model_values) - 1))
        elem_size = model_values.dtype.itemsize
        s = data.draw(keywords(size=elem_size))

        actual_kw[i] = s
        model_values[i] = s.encode()

        assert actual_kw[i] == s

    @rule(data=st.data(), kw=st.one_of(numeric_kws, str_kws))
    def sub_copy(self, data, kw):
        actual_kw, (model_name, model_values) = kw
        n = len(model_values)
        assume(n >= 1)
        offset = data.draw(st.integers(min_value=0, max_value=n - 1))
        count = data.draw(st.integers(min_value=1, max_value=n - offset))
        new_header = data.draw(st.one_of(st.none(), keywords(size=8)))

        sub = actual_kw.sub_copy(offset, count, new_header=new_header)
        model_slice = model_values[offset : offset + count]

        assert sub.name == (new_header if new_header is not None else model_name)

        if model_values.dtype == np.int32:
            npt.assert_equal(sub.numpy_view(), model_slice)
        elif actual_kw.is_numeric():
            npt.assert_allclose(sub.numpy_view(), model_slice, rtol=1e-2, atol=1e-6)
        else:
            assert [sub[i] for i in range(count)] == [m.decode() for m in model_slice]

    @rule(kw1=numeric_kws, kw2=numeric_kws)
    def assign_kw(self, kw1, kw2):
        akw1, (_, model_values1) = kw1
        akw2, (_, model_values2) = kw2
        assume(akw1.assert_binary(akw2))

        akw1.assign(akw2)
        model_values1[:] = model_values2

        if model_values1.dtype == np.int32:
            npt.assert_equal(akw1.numpy_view(), model_values1)
            npt.assert_equal(akw2.numpy_view(), model_values2)
        else:
            npt.assert_allclose(akw1.numpy_view(), model_values1, rtol=1e-2, atol=1e-6)
            npt.assert_allclose(akw2.numpy_view(), model_values2, rtol=1e-2, atol=1e-6)

    @rule(data=st.data(), kw=numeric_kws)
    def iadd_scalar(self, data, kw):
        actual_kw, (_, model_values) = kw
        actual_delta, delta = self.draw_scalar(data, model_values.dtype)

        actual_kw += actual_delta
        model_values += delta

        size_order = max(1.0, abs(delta), order_of(model_values))
        npt.assert_allclose(
            actual_kw.numpy_view(), model_values, atol=size_order * 1e-6
        )

    @rule(data=st.data(), kw=numeric_kws)
    def isub_scalar(self, data, kw):
        actual_kw, (_, model_values) = kw
        actual_delta, delta = self.draw_scalar(data, model_values.dtype)

        # Avoid overflow on negation
        assume(delta > -(2**31))

        actual_kw -= actual_delta
        model_values -= delta

        size_order = max(1.0, abs(delta), order_of(model_values)) + 16
        npt.assert_allclose(
            actual_kw.numpy_view(), model_values, atol=size_order * 1e-6
        )

    @rule(data=st.data(), kw=numeric_kws)
    def imul_scalar(self, data, kw):
        actual_kw, (_, model_values) = kw
        actual_factor, factor = self.draw_scalar(data, model_values.dtype)

        actual_kw *= actual_factor
        model_values *= factor

        size_order = max(1.0, abs(factor), order_of(model_values))
        npt.assert_allclose(
            actual_kw.numpy_view(), model_values, atol=size_order * 1e-6
        )

    @rule(kw1=numeric_kws, kw2=numeric_kws)
    def isub(self, kw1, kw2):
        akw1, (_, model_values1) = kw1
        akw2, (_, model_values2) = kw2
        assume(akw1.assert_binary(akw2))

        akw1 -= akw2
        model_values1 -= model_values2

        size_order = max(1.0, order_of(model_values1), order_of(model_values2))
        npt.assert_allclose(akw1.numpy_view(), model_values1, atol=size_order * 1e-6)
        npt.assert_allclose(akw2.numpy_view(), model_values2, atol=size_order * 1e-6)

    @rule(data=st.data(), kw=numeric_kws)
    def add(self, data, kw):
        actual_kw, (_, model_values) = kw
        actual_delta, delta = self.draw_scalar(data, model_values.dtype)

        new_kw = actual_kw + actual_delta
        expected = model_values + delta

        size_order = max(1.0, abs(delta), order_of(model_values)) + 16
        npt.assert_allclose(new_kw.numpy_view(), expected, atol=size_order * 1e-6)
        npt.assert_allclose(
            actual_kw.numpy_view(), model_values, atol=size_order * 1e-6
        )

    @rule(data=st.data(), kw=numeric_kws)
    def radd_returns_copy(self, data, kw):
        actual_kw, (_, model_values) = kw
        actual_delta, delta = self.draw_scalar(data, model_values.dtype)

        new_kw = actual_delta + actual_kw
        expected = delta + model_values

        size_order = max(1.0, abs(delta), order_of(model_values)) + 16
        npt.assert_allclose(new_kw.numpy_view(), expected, atol=size_order * 1e-6)
        npt.assert_allclose(
            actual_kw.numpy_view(), model_values, atol=size_order * 1e-6
        )

    @rule(data=st.data(), kw=numeric_kws)
    def sub(self, data, kw):
        actual_kw, (_, model_values) = kw
        actual_delta, delta = self.draw_scalar(data, model_values.dtype)

        # Avoid overflow on negation
        assume(delta > -(2**31))

        new_kw = actual_kw - actual_delta
        expected = model_values - delta

        size_order = max(1.0, abs(delta), order_of(model_values)) + 16
        npt.assert_allclose(new_kw.numpy_view(), expected, atol=size_order * 1e-6)
        npt.assert_allclose(
            actual_kw.numpy_view(), model_values, atol=size_order * 1e-6
        )

    @rule(data=st.data(), kw=numeric_kws)
    def rsub(self, data, kw):
        actual_kw, (_, model_values) = kw
        actual_delta, delta = self.draw_scalar(data, model_values.dtype)

        # Avoid overflow on negation
        assume(delta > -(2**31))

        new_kw = actual_delta - actual_kw
        # __rsub__ is implemented as (self - delta) * -1
        expected = (model_values - delta) * -1

        size_order = max(1.0, abs(delta), order_of(model_values)) + 16
        npt.assert_allclose(new_kw.numpy_view(), expected, atol=size_order * 1e-6)
        npt.assert_allclose(
            actual_kw.numpy_view(), model_values, atol=size_order * 1e-6
        )

    @rule(data=st.data(), kw=numeric_kws)
    def mul(self, data, kw):
        actual_kw, (_, model_values) = kw
        actual_factor, factor = self.draw_scalar(data, model_values.dtype)

        new_kw = actual_kw * actual_factor
        expected = model_values * factor

        size_order = max(1.0, abs(factor), order_of(model_values))
        npt.assert_allclose(new_kw.numpy_view(), expected, atol=size_order * 1e-6)
        npt.assert_allclose(
            actual_kw.numpy_view(), model_values, atol=size_order * 1e-6
        )

    @rule(data=st.data(), kw=numeric_kws)
    def rmul(self, data, kw):
        actual_kw, (_, model_values) = kw
        actual_factor, factor = self.draw_scalar(data, model_values.dtype)

        new_kw = actual_factor * actual_kw
        expected = factor * model_values

        size_order = max(1.0, abs(factor), order_of(model_values))
        npt.assert_allclose(new_kw.numpy_view(), expected, atol=size_order * 1e-6)
        npt.assert_allclose(
            actual_kw.numpy_view(), model_values, atol=size_order * 1e-6
        )

    @rule(kw=numeric_kws)
    def abs(self, kw):
        actual_kw, (_, model_values) = kw
        npt.assert_allclose(abs(actual_kw), np.abs(model_values), rtol=1e-2, atol=1e-6)

    @rule(kw=st.one_of(numeric_kws, str_kws))
    def deep_copy(self, kw):
        actual_kw, _ = kw
        cp = actual_kw.deep_copy()
        assert actual_kw.equal(cp)

    @rule(kw=numeric_kws)
    def equal_numeric_self(self, kw):
        actual_kw, _ = kw
        assert actual_kw.equal_numeric(actual_kw)
        assert actual_kw.equal_numeric(actual_kw.copy())

    @rule(kw=numeric_kws)
    def first_different_self(self, kw):
        actual_kw, _ = kw
        cp = actual_kw.copy()
        assume(len(actual_kw))
        assert actual_kw.first_different(cp) == len(actual_kw)


TestKw = StatefulKwTest.TestCase


def test_create_negative_size_raises():
    with pytest.raises(TypeError):
        ResdataKW("KW", -1, ResDataType.RD_INT)


def test_set_too_long_string_raises():
    kw = ResdataKW("S", 2, ResDataType.RD_STRING(8))
    with pytest.raises(ValueError, match="cannot hold input string of length 9"):
        kw[0] = "123456789"


def test_fread_formatted_corrupt_data_raises(use_tmpdir):
    kw = ResdataKW("INTKW", 4, ResDataType.RD_INT)
    for i in range(len(kw)):
        kw[i] = i
    with openFortIO("F.txt", FortIO.WRITE_MODE, fmt_file=True) as f:
        kw.fwrite(f)

    with open("F.txt") as inf, open("BAD.txt", "w") as outf:
        outf.write(inf.read().replace("0", "XYZ", 1))

    with pytest.raises(RuntimeError, match="reading of keyword:INTKW"):
        with openFortIO("BAD.txt", fmt_file=True) as f:
            ResdataKW.fread(f)


def test_fread_formatted_bad_logical_raises(use_tmpdir):
    kw = ResdataKW("BKW", 3, ResDataType.RD_BOOL)
    for i in range(len(kw)):
        kw[i] = True
    with openFortIO("B.txt", FortIO.WRITE_MODE, fmt_file=True) as f:
        kw.fwrite(f)

    with open("B.txt") as inf, open("BAD.txt", "w") as outf:
        outf.write(inf.read().replace("T", "Q"))

    with pytest.raises(RuntimeError, match=r"Logical value: \[Q\] not recogniced"):
        with openFortIO("BAD.txt", fmt_file=True) as f:
            ResdataKW.fread(f)


def _roundtrip(kw, path, fmt_file):
    """Write `kw` to `path` with the given format and read it back."""
    with openFortIO(str(path), mode=FortIO.WRITE_MODE, fmt_file=fmt_file) as fortio:
        kw.fwrite(fortio)
    with openFortIO(str(path), fmt_file=fmt_file) as fortio:
        return ResdataKW.fread(fortio)


def test_that_bool_keywords_are_stored_as_integers(tmp_path):
    kw = ResdataKW("BOOLKW", 2, ResDataType.RD_BOOL)
    kw[0] = True
    kw[1] = False

    path = tmp_path / "bool.kw"
    with openFortIO(str(path), mode=FortIO.WRITE_MODE) as fortio:
        kw.fwrite(fortio)

    # The trailing fortran record holds the data; True is encoded as
    # 0xFFFFFFFF (-1) and False as 0.
    data_record = path.read_bytes()[-16:]
    assert data_record[4:8] == b"\xff\xff\xff\xff"
    assert data_record[8:12] == b"\x00\x00\x00\x00"


@pytest.mark.parametrize("fmt_file", [False, True], ids=["unformatted", "formatted"])
def test_that_bool_keywords_roundtrip(tmp_path, fmt_file):
    values = [True, False, True, True, False]
    kw = ResdataKW("BOOLKW", len(values), ResDataType.RD_BOOL)
    for i, value in enumerate(values):
        kw[i] = value

    loaded = _roundtrip(kw, tmp_path / "bool.kw", fmt_file)

    assert loaded.name == "BOOLKW"
    assert loaded.data_type.is_bool()
    assert list(loaded) == values


@pytest.mark.parametrize("data_type", [ResDataType.RD_FLOAT, ResDataType.RD_DOUBLE])
@pytest.mark.parametrize("fmt_file", [False, True], ids=["unformatted", "formatted"])
def test_that_floating_keywords_roundtrip(data_type, tmp_path, fmt_file):
    values = [0.0, -1.5, 1234.5, 1.0e-7, -9.75e10]
    kw = ResdataKW("FLOATKW", len(values), data_type)
    for i, value in enumerate(values):
        kw[i] = value

    loaded = _roundtrip(kw, tmp_path / "float.kw", fmt_file)

    assert loaded.data_type.is_float() == data_type.is_float()
    assert loaded.data_type.is_double() == data_type.is_double()
    npt.assert_allclose(loaded.numpy_view(), np.array(values, dtype=np.float32))


@pytest.mark.parametrize("fmt_file", [False, True], ids=["unformatted", "formatted"])
def test_that_char_keywords_rountrip(tmp_path, fmt_file):
    values = ["A", "AB", "ABCDEFGH", ""]
    kw = ResdataKW("CHARKW", len(values), ResDataType.RD_CHAR)
    for i, value in enumerate(values):
        kw[i] = value

    loaded = _roundtrip(kw, tmp_path / "char.kw", fmt_file)

    assert loaded.data_type.is_char()
    assert [value.strip() for value in loaded] == values


def test_that_char_elements_shorter_than_eight_are_space_padded_when_written(tmp_path):
    kw = ResdataKW("CHARKW", 1, ResDataType.RD_CHAR)
    kw[0] = "AB"

    path = tmp_path / "char.kw"
    with openFortIO(str(path), mode=FortIO.WRITE_MODE) as fortio:
        kw.fwrite(fortio)

    assert path.read_bytes()[-12:-4] == b"AB      "


@pytest.mark.parametrize("fmt_file", [False, True], ids=["unformatted", "formatted"])
def test_that_message_keywords_roundtrip_header_only(tmp_path, fmt_file):
    kw = ResdataKW("MESSKW", 0, ResDataType.RD_MESS)

    loaded = _roundtrip(kw, tmp_path / "mess.kw", fmt_file)

    assert loaded.name == "MESSKW"
    assert loaded.data_type.is_mess()
    assert len(loaded) == 0


def test_that_grdecl_output_of_float_keyword_roundtrips(tmp_path):
    values = [0.0, 1.5, -2.25, 1.0e-7, 3.0e10]
    kw = ResdataKW("PORO", len(values), ResDataType.RD_FLOAT)
    for i, value in enumerate(values):
        kw[i] = value

    path = tmp_path / "poro.grdecl"
    with open(path, "w") as f:
        kw.write_grdecl(f)
    with open(path) as f:
        loaded = ResdataKW.read_grdecl(f, "PORO", rd_type=ResDataType.RD_FLOAT)

    npt.assert_allclose(loaded.numpy_view(), np.array(values, dtype=np.float32))


def test_that_numeric_keywords_support_elementwise_arithmetic():
    lhs = ResdataKW("LHS", 3, ResDataType.RD_DOUBLE)
    rhs = ResdataKW("RHS", 3, ResDataType.RD_DOUBLE)
    for i, (left, right) in enumerate([(10.0, 2.0), (-3.0, 4.0), (0.0, 5.0)]):
        lhs[i] = left
        rhs[i] = right

    added = lhs.copy()
    added += rhs
    npt.assert_allclose(added.numpy_view(), [12.0, 1.0, 5.0])

    subtracted = lhs.copy()
    subtracted -= rhs
    npt.assert_allclose(subtracted.numpy_view(), [8.0, -7.0, -5.0])

    multiplied = lhs.copy()
    multiplied *= rhs
    npt.assert_allclose(multiplied.numpy_view(), [20.0, -12.0, 0.0])

    divided = lhs.copy()
    divided.div(rhs)
    npt.assert_allclose(divided.numpy_view(), [5.0, -0.75, 0.0])


def test_that_numeric_keywords_can_be_divided_elementwise():
    lhs = ResdataKW("LHS", 3, ResDataType.RD_FLOAT)
    rhs = ResdataKW("RHS", 3, ResDataType.RD_FLOAT)
    for i, (left, right) in enumerate([(1.0, 2.0), (-3.0, 4.0), (9.0, 3.0)]):
        lhs[i] = left
        rhs[i] = right

    lhs.div(rhs)

    npt.assert_allclose(lhs.numpy_view(), [0.5, -0.75, 3.0])


def test_that_integer_keyword_division_truncates_towards_zero():
    lhs = ResdataKW("LHS", 3, ResDataType.RD_INT)
    rhs = ResdataKW("RHS", 3, ResDataType.RD_INT)
    for i, (left, right) in enumerate([(7, 2), (-7, 2), (6, 3)]):
        lhs[i] = left
        rhs[i] = right

    lhs.div(rhs)

    assert list(lhs) == [3, -3, 2]


def test_that_isqrt_rounds_int_elements_to_the_nearest_square_root():
    kw = ResdataKW("INTKW", 5, ResDataType.RD_INT)
    for i, value in enumerate([0, 1, 9, 15, 16]):
        kw[i] = value

    kw.isqrt()

    # sqrt(15) == 3.87... is rounded to 4 rather than truncated.
    assert list(kw) == [0, 1, 3, 4, 4]


def test_that_isqrt_replaces_double_elements_with_their_square_root():
    kw = ResdataKW("DOUBLEKW", 3, ResDataType.RD_DOUBLE)
    for i, value in enumerate([0.0, 4.0, 2.0]):
        kw[i] = value

    kw.isqrt()

    npt.assert_allclose(kw.numpy_view(), [0.0, 2.0, np.sqrt(2.0)])


def test_that_abs_returns_absolute_values_without_modifying_the_original():
    kw = ResdataKW("DOUBLEKW", 3, ResDataType.RD_DOUBLE)
    for i, value in enumerate([-1.5, 0.0, 2.5]):
        kw[i] = value

    result = abs(kw)

    npt.assert_allclose(result.numpy_view(), [1.5, 0.0, 2.5])
    npt.assert_allclose(kw.numpy_view(), [-1.5, 0.0, 2.5])


def test_that_abs_of_int_keyword_returns_absolute_values():
    kw = ResdataKW("INTKW", 3, ResDataType.RD_INT)
    for i, value in enumerate([-2, 0, 3]):
        kw[i] = value

    assert list(abs(kw)) == [2, 0, 3]


def test_that_add_squared_accumulates_squares_of_keyword():
    target = ResdataKW("TARGET", 3, ResDataType.RD_DOUBLE)
    other = ResdataKW("OTHER", 3, ResDataType.RD_DOUBLE)
    for i, value in enumerate([1.0, -2.0, 3.0]):
        target[i] = 1.0
        other[i] = value

    target.add_squared(other)

    npt.assert_allclose(target.numpy_view(), [2.0, 5.0, 10.0])


def test_that_scalar_shift_and_scale_applies_elementwize():
    kw = ResdataKW("DOUBLEKW", 3, ResDataType.RD_DOUBLE)
    for i, value in enumerate([1.0, 2.0, 3.0]):
        kw[i] = value

    kw += 0.5
    npt.assert_allclose(kw.numpy_view(), [1.5, 2.5, 3.5])

    kw *= 2.0
    npt.assert_allclose(kw.numpy_view(), [3.0, 5.0, 7.0])

    kw -= 1.0
    npt.assert_allclose(kw.numpy_view(), [2.0, 4.0, 6.0])


def test_that_assign_sets_every_element_of_a_keyword():
    kw = ResdataKW("DOUBLEKW", 4, ResDataType.RD_DOUBLE)

    kw.assign(3.25)

    npt.assert_allclose(kw.numpy_view(), [3.25] * 4)


def test_that_assign_from_another_keyword_copies_its_data():
    source = ResdataKW("SRC", 3, ResDataType.RD_DOUBLE)
    target = ResdataKW("TARGET", 3, ResDataType.RD_DOUBLE)
    for i, value in enumerate([1.0, 2.0, 3.0]):
        source[i] = value

    target.assign(source)

    npt.assert_allclose(target.numpy_view(), [1.0, 2.0, 3.0])
    assert target.name == "TARGET"


def test_that_sum_of_a_keyword_is_the_sum_of_its_elements():
    kw = ResdataKW("DOUBLEKW", 3, ResDataType.RD_DOUBLE)
    for i, value in enumerate([1.5, 2.5, -1.0]):
        kw[i] = value

    assert kw.sum() == pytest.approx(3.0)


@pytest.mark.parametrize(
    "data_type, values",
    [
        (ResDataType.RD_INT, [3, -1, 7, 0]),
        (ResDataType.RD_FLOAT, [3.5, -1.5, 7.25, 0.0]),
        (ResDataType.RD_DOUBLE, [3.5, -1.5, 7.25, 0.0]),
    ],
    ids=["int", "float", "double"],
)
def test_that_get_min_max_returns_the_extreme_elements(data_type, values):
    kw = ResdataKW("KW", len(values), data_type)
    for i, value in enumerate(values):
        kw[i] = value

    assert kw.get_min_max() == (pytest.approx(min(values)), pytest.approx(max(values)))
    assert kw.get_min() == pytest.approx(min(values))
    assert kw.get_max() == pytest.approx(max(values))


def _region_kw(values):
    kw = ResdataKW("REGIONS", len(values), ResDataType.RD_INT)
    for i, value in enumerate(values):
        kw[i] = value
    return kw


def test_that_fix_uninitialized_fills_a_hole_where_all_neighbours_agree():
    grid = GridGenerator.create_rectangular((3, 3, 1), (1, 1, 1))
    # The centre cell is uninitialized, all its xy neighbours are region 7.
    kw = _region_kw([7, 7, 7, 7, 0, 7, 7, 7, 7])

    kw.fix_uninitialized(grid)

    assert list(kw) == [7] * 9


def test_that_fix_uninitialized_leaves_cells_with_disagreeing_neighbours():
    grid = GridGenerator.create_rectangular((3, 3, 1), (1, 1, 1))
    # The west neighbour says 1 and the east neighbour says 2, so the
    # heuristic must not pick a value for the centre cell.
    kw = _region_kw([0, 0, 0, 1, 0, 2, 0, 0, 0])

    kw.fix_uninitialized(grid)

    assert kw[4] == 0


def test_that_fix_uninitialized_ignores_inactive_neighbours():
    # Only the cells in the middle column are active.
    actnum = [0, 1, 0] * 3
    grid = GridGenerator.create_rectangular((3, 3, 1), (1, 1, 1), actnum=actnum)
    kw = _region_kw([9, 3, 9, 9, 0, 9, 9, 3, 9])

    kw.fix_uninitialized(grid)

    # The 9s are inactive and must not contribute; the active north/south
    # neighbours both hold 3.
    assert kw[4] == 3


def test_that_fix_uninitialized_propagates_values_across_several_passes():
    grid = GridGenerator.create_rectangular((5, 1, 1), (1, 1, 1))
    kw = _region_kw([4, 0, 0, 0, 0])

    kw.fix_uninitialized(grid)

    assert list(kw) == [4, 4, 4, 4, 4]


def test_that_fix_uninitialized_is_applied_layer_by_layer():
    grid = GridGenerator.create_rectangular((3, 3, 2), (1, 1, 1))
    # Layer 0 can be resolved, layer 1 is entirely uninitialized.
    kw = _region_kw([7, 7, 7, 7, 0, 7, 7, 7, 7] + [0] * 9)

    kw.fix_uninitialized(grid)

    assert list(kw)[:9] == [7] * 9
    assert list(kw)[9:] == [0] * 9


def test_that_create_actnum_marks_cells_strictly_above_the_pore_volume_limit():
    porv = ResdataKW("PORV", 4, ResDataType.RD_FLOAT)
    for i, value in enumerate([-1.0, 0.0, 0.5, 100.0]):
        porv[i] = value

    assert list(porv.create_actnum()) == [0, 0, 1, 1]
    assert list(porv.create_actnum(porv_limit=1.0)) == [0, 0, 0, 1]
    assert porv.create_actnum().name == "ACTNUM"


@pytest.mark.parametrize("fmt_file", [False, True], ids=["unformatted", "formatted"])
def test_that_char_keywords_spanning_several_records_roundtrip(tmp_path, fmt_file):
    # Alpha keywords are written in blocks of 105 elements.
    size = 250
    kw = ResdataKW("CHARKW", size, ResDataType.RD_CHAR)
    for i in range(size):
        kw[i] = "E%04d" % i

    loaded = _roundtrip(kw, tmp_path / "char.kw", fmt_file)

    assert [value.strip() for value in loaded] == ["E%04d" % i for i in range(size)]


@pytest.mark.parametrize("fmt_file", [False, True], ids=["unformatted", "formatted"])
def test_that_double_keywords_spanning_several_records_roundtrip(tmp_path, fmt_file):
    # Numeric keywords are written in blocks of 1000 elements.
    size = 2500
    kw = ResdataKW("DOUBLEKW", size, ResDataType.RD_DOUBLE)
    values = np.linspace(-1.0, 1.0, size)
    kw.numpy_view()[:] = values

    loaded = _roundtrip(kw, tmp_path / "double.kw", fmt_file)

    npt.assert_allclose(loaded.numpy_view(), values)


def test_that_keywords_can_be_written_and_read_back_in_order(tmp_path):
    first = ResdataKW("FIRST", 2, ResDataType.RD_INT)
    second = ResdataKW("SECOND", 3, ResDataType.RD_DOUBLE)
    third = ResdataKW("THIRD", 1, ResDataType.RD_BOOL)
    third[0] = True

    path = tmp_path / "multi.kw"
    with openFortIO(str(path), mode=FortIO.WRITE_MODE) as fortio:
        for kw in (first, second, third):
            kw.fwrite(fortio)

    with openFortIO(str(path)) as fortio:
        kws = [ResdataKW.fread(fortio) for _ in range(3)]

    assert [k.name for k in kws] == ["FIRST", "SECOND", "THIRD"]
    assert kws[2][0] is True


def test_that_fort_io_size_is_the_number_of_bytes_written(tmp_path):
    kw = ResdataKW("INTKW", 7, ResDataType.RD_INT)

    path = tmp_path / "int.kw"
    with openFortIO(str(path), mode=FortIO.WRITE_MODE) as fortio:
        kw.fwrite(fortio)

    assert kw.fort_io_size() == path.stat().st_size


def test_that_message_keywords_roundtrip(tmp_path):
    kw = ResdataKW("MESSKW", 3, ResDataType.RD_MESS)

    loaded = _roundtrip(kw, tmp_path / "mess.kw", fmt_file=False)

    assert loaded.name == "MESSKW"
    assert loaded.type_name() == "MESS"
    assert len(loaded) == 3


def test_that_message_keywords_with_data_cannot_be_written_formatted(tmp_path):
    kw = ResdataKW("MESSKW", 3, ResDataType.RD_MESS)

    with pytest.raises(
        RuntimeError, match="message type keywords should not have data"
    ):
        with openFortIO(
            str(tmp_path / "mess.txt"), mode=FortIO.WRITE_MODE, fmt_file=True
        ) as fortio:
            kw.fwrite(fortio)


def test_that_formatted_message_keywords_are_read_as_quoted_strings(tmp_path):
    path = tmp_path / "mess.txt"
    path.write_text(" 'MESSKW  '           3 'MESS'\n" + "HELLO   WORLD   AGAIN   \n")

    with openFortIO(str(path), fmt_file=True) as fortio:
        kw = ResdataKW.fread(fortio)

    assert kw.name == "MESSKW"
    assert kw.type_name() == "MESS"
    assert len(kw) == 3


def test_that_formatted_float_keyword_with_corrupt_data_raises(tmp_path):
    kw = ResdataKW("FLOATKW", 3, ResDataType.RD_FLOAT)
    path = tmp_path / "float.txt"
    with openFortIO(str(path), mode=FortIO.WRITE_MODE, fmt_file=True) as fortio:
        kw.fwrite(fortio)

    corrupt = tmp_path / "corrupt.txt"
    corrupt.write_text(path.read_text().replace("0.000", "XYZ", 1))

    with pytest.raises(RuntimeError, match="reading of keyword:FLOATKW"):
        with openFortIO(str(corrupt), fmt_file=True) as fortio:
            ResdataKW.fread(fortio)


ALPHA_TYPES = [ResDataType.RD_CHAR, ResDataType.RD_STRING(5)]
ALPHA_IDS = ["char", "string"]
ALL_TYPES = [
    ResDataType.RD_INT,
    ResDataType.RD_FLOAT,
    ResDataType.RD_DOUBLE,
    ResDataType.RD_BOOL,
    ResDataType.RD_CHAR,
    ResDataType.RD_STRING(5),
]
ALL_IDS = ["int", "float", "double", "bool", "char", "string"]


def _filled_kw(data_type, size=6):
    kw = ResdataKW("KW", size, data_type)
    for i in range(size):
        if data_type.is_bool():
            kw[i] = i % 2 == 0
        elif data_type.is_char() or data_type.is_string():
            kw[i] = "E%d" % i
        else:
            kw[i] = i
    return kw


@pytest.mark.parametrize("data_type", ALL_TYPES, ids=ALL_IDS)
def test_that_strided_slicing_selects_every_nth_element(data_type):
    kw = _filled_kw(data_type)

    sliced = kw[0:6:2]

    assert len(sliced) == 3
    assert list(sliced) == [kw[0], kw[2], kw[4]]


@pytest.mark.parametrize("data_type", ALL_TYPES, ids=ALL_IDS)
def test_that_sub_copy_extracts_a_contiguous_block(data_type):
    kw = _filled_kw(data_type)

    copy = kw.sub_copy(2, 3, new_header="SUB")

    assert copy.name == "SUB"
    assert list(copy) == [kw[2], kw[3], kw[4]]


def test_that_sub_copy_with_negative_count_copies_the_remaining_elements():
    source = ResdataKW("SRC", 5, ResDataType.RD_INT)
    for i in range(5):
        source[i] = i

    copy = source.sub_copy(2, -1, new_header="TAIL")

    assert copy.name == "TAIL"
    assert list(copy) == [2, 3, 4]


@pytest.mark.parametrize("data_type", ALL_TYPES, ids=ALL_IDS)
def test_that_resize_keeps_the_prefix(data_type):
    kw = _filled_kw(data_type, size=3)
    original = list(kw)

    kw.resize(5)

    assert len(kw) == 5
    assert list(kw)[:3] == original


def test_that_first_different_reports_the_index_of_the_first_unequal_element():
    lhs = ResdataKW("LHS", 4, ResDataType.RD_INT)
    rhs = ResdataKW("RHS", 4, ResDataType.RD_INT)
    for i in range(4):
        lhs[i] = i
        rhs[i] = i
    rhs[2] = 100

    assert lhs.first_different(rhs) == 2


def test_that_first_different_returns_the_size_for_equal_keywords():
    lhs = ResdataKW("LHS", 4, ResDataType.RD_INT)
    rhs = ResdataKW("RHS", 4, ResDataType.RD_INT)
    for i in range(4):
        lhs[i] = i
        rhs[i] = i

    assert lhs.first_different(rhs) == 4


@pytest.mark.parametrize(
    "data_type", [ResDataType.RD_FLOAT, ResDataType.RD_DOUBLE], ids=["float", "double"]
)
def test_that_first_different_with_epsilon_compares_approximately(data_type):
    lhs = ResdataKW("LHS", 3, data_type)
    rhs = ResdataKW("RHS", 3, data_type)
    for i, value in enumerate([1.0, 2.0, 3.0]):
        lhs[i] = value
        rhs[i] = value
    rhs[1] = 2.0 + 1e-5
    rhs[2] = 4.0

    assert lhs.first_different(rhs) == 1
    assert lhs.first_different(rhs, epsilon=1e-3) == 2


def test_that_first_different_honours_the_offset_argument():
    lhs = ResdataKW("LHS", 4, ResDataType.RD_INT)
    rhs = ResdataKW("RHS", 4, ResDataType.RD_INT)
    for i in range(4):
        lhs[i] = i
        rhs[i] = i
    rhs[0] = 100
    rhs[3] = 100

    assert lhs.first_different(rhs) == 0
    assert lhs.first_different(rhs, offset=1) == 3


@pytest.mark.parametrize("data_type", ALPHA_TYPES, ids=ALPHA_IDS)
def test_that_first_different_compares_alphanumeric_keywords_elementwise(data_type):
    lhs = _filled_kw(data_type)
    rhs = _filled_kw(data_type)

    assert lhs.first_different(rhs) == len(lhs)

    rhs[3] = "XX"
    assert lhs.first_different(rhs) == 3


def test_that_reading_past_the_end_of_a_file_raises(tmp_path):
    path = tmp_path / "empty.kw"
    path.write_bytes(b"")

    with pytest.raises(ValueError, match="Failed to create ResdataKW instance"):
        with openFortIO(str(path)) as fortio:
            ResdataKW.fread(fortio)


@pytest.mark.parametrize(
    "token",
    ["0.10000000000000E+01", "D+01", "0.1000000000000xD+01"],
    ids=["no-d-marker", "no-mantissa", "bad-mantissa"],
)
def test_that_formatted_double_keyword_with_a_malformed_token_raises(tmp_path, token):
    path = tmp_path / "double.txt"
    path.write_text(" 'DOUBLEKW'           1 'DOUB'\n  %s\n" % token)

    with pytest.raises(RuntimeError, match="read failed"):
        with openFortIO(str(path), fmt_file=True) as fortio:
            ResdataKW.fread(fortio)


def test_that_formatted_double_keyword_truncated_before_the_data_raises(tmp_path):
    path = tmp_path / "double.txt"
    path.write_text(" 'DOUBLEKW'           1 'DOUB'\n")

    with pytest.raises(RuntimeError, match="read failed"):
        with openFortIO(str(path), fmt_file=True) as fortio:
            ResdataKW.fread(fortio)


def test_that_formatted_bool_keyword_truncated_before_the_data_raises(tmp_path):
    path = tmp_path / "bool.txt"
    path.write_text(" 'BOOLKW  '           1 'LOGI'\n")

    with pytest.raises(RuntimeError, match="premature file end"):
        with openFortIO(str(path), fmt_file=True) as fortio:
            ResdataKW.fread(fortio)


@pytest.mark.parametrize(
    "data_type",
    [ResDataType.RD_INT, ResDataType.RD_BOOL, ResDataType.RD_CHAR],
    ids=["int", "bool", "char"],
)
def test_that_equal_numeric_falls_back_to_exact_comparison_for_non_float_types(
    data_type,
):
    lhs = _filled_kw(data_type, size=3)
    rhs = _filled_kw(data_type, size=3)

    assert lhs.equal_numeric(rhs, epsilon=1e-3)

    rhs[2] = "XX" if data_type.is_char() else (not lhs[2] if data_type.is_bool() else 7)
    assert not lhs.equal_numeric(rhs, epsilon=1e-3)


def test_that_message_keywords_compare_equal_when_headers_match():
    lhs = ResdataKW("MESSKW", 3, ResDataType.RD_MESS)
    rhs = ResdataKW("MESSKW", 3, ResDataType.RD_MESS)
    other = ResdataKW("OTHERKW", 3, ResDataType.RD_MESS)

    assert lhs == rhs
    assert lhs.equal_numeric(rhs)
    assert lhs != other


@pytest.mark.parametrize(
    "data_type", [ResDataType.RD_FLOAT, ResDataType.RD_DOUBLE], ids=["float", "double"]
)
def test_that_equal_numeric_with_epsilon_compares_approximately(data_type):
    lhs = ResdataKW("KW", 3, data_type)
    rhs = ResdataKW("KW", 3, data_type)
    for i, value in enumerate([1.0, 2.0, 3.0]):
        lhs[i] = value
        rhs[i] = value + 1e-5

    assert lhs.equal_numeric(rhs, epsilon=1e-3)
    assert not lhs.equal_numeric(rhs, abs_epsilon=1e-12, rel_epsilon=1e-12)
    assert not lhs.equal(rhs)


def test_that_equal_distinguishes_keywords_differing_in_a_single_element():
    lhs = ResdataKW("KW", 3, ResDataType.RD_INT)
    rhs = ResdataKW("KW", 3, ResDataType.RD_INT)
    for i in range(3):
        lhs[i] = i
        rhs[i] = i

    assert lhs.equal(rhs)

    rhs[2] = 99
    assert not lhs.equal(rhs)


@pytest.mark.parametrize("data_type", ALL_TYPES, ids=ALL_IDS)
def test_that_two_empty_keywords_of_the_same_type_are_equal(data_type):
    assert ResdataKW("EMPTY", 0, data_type).equal(ResdataKW("EMPTY", 0, data_type))


@pytest.mark.parametrize("data_type", ALL_TYPES, ids=ALL_IDS)
def test_that_an_empty_keyword_differs_from_a_non_empty_one_of_the_same_type(
    data_type,
):
    assert not ResdataKW("EMPTY", 0, data_type).equal(_filled_kw(data_type, size=1))


@pytest.mark.parametrize("data_type", ALL_TYPES, ids=ALL_IDS)
def test_that_scatter_copy_places_elements_on_the_active_cells(data_type):
    src = _filled_kw(data_type, size=2)
    actnum = ResdataKW("ACTNUM", 4, ResDataType.RD_INT)
    actnum[1] = 1
    actnum[3] = 1

    scattered = src.scatter_copy(actnum)

    assert len(scattered) == 4
    assert scattered[1] == src[0]
    assert scattered[3] == src[1]


@pytest.mark.parametrize("data_type", ALL_TYPES, ids=ALL_IDS)
def test_that_scatter_copy_rejects_an_actnum_with_too_few_active_cells(data_type):
    src = _filled_kw(data_type, size=3)
    actnum = ResdataKW("ACTNUM", 2, ResDataType.RD_INT)
    actnum[0] = 1
    actnum[1] = 1

    with pytest.raises(ValueError, match="Failed to create ResdataKW instance"):
        src.scatter_copy(actnum)


@pytest.mark.parametrize("data_type", ALL_TYPES, ids=ALL_IDS)
def test_that_scatter_copy_rejects_an_actnum_with_more_active_cells_than_elements(
    data_type,
):
    src = _filled_kw(data_type, size=1)
    actnum = ResdataKW("ACTNUM", 3, ResDataType.RD_INT)
    actnum[0] = 1
    actnum[2] = 1

    with pytest.raises(ValueError, match="Failed to create ResdataKW instance"):
        src.scatter_copy(actnum)


def _write_two_keywords(path):
    """Writes two INTE keywords and returns the offset of the second one."""
    with openFortIO(str(path), mode=FortIO.WRITE_MODE) as fortio:
        _int_kw("KW1", [0, 1, 2, 3]).fwrite(fortio)
        offset = fortio.get_position()
        _int_kw("KW2", [4, 5, 6, 7]).fwrite(fortio)
    return offset


def _int_kw(name, values):
    kw = ResdataKW(name, len(values), ResDataType.RD_INT)
    for index, value in enumerate(values):
        kw[index] = value
    return kw


def test_that_a_keyword_can_be_written_back_in_place(tmp_path):
    path = tmp_path / "file"
    _write_two_keywords(path)

    rd_file = ResdataFile(str(path), flags=FileMode.WRITABLE)
    kw = rd_file[1]
    kw[0] = 42
    rd_file.save_kw(kw)
    rd_file.close()

    assert list(ResdataFile(str(path))[1]) == [42, 5, 6, 7]


def test_that_a_keyword_can_be_loaded_after_the_stream_has_been_closed(tmp_path):
    path = tmp_path / "file"
    _write_two_keywords(path)

    rd_file = ResdataFile(str(path))
    rd_file.close()

    assert list(rd_file[0]) == [0, 1, 2, 3]


def test_that_keywords_can_be_loaded_when_the_stream_is_closed_between_reads(tmp_path):
    path = tmp_path / "file"
    _write_two_keywords(path)

    rd_file = ResdataFile(str(path), flags=FileMode.CLOSE_STREAM)

    assert list(rd_file[0]) == [0, 1, 2, 3]
    assert list(rd_file[1]) == [4, 5, 6, 7]


def test_that_reading_past_the_last_keyword_fails(tmp_path):
    path = tmp_path / "file"
    _write_two_keywords(path)

    with openFortIO(str(path)) as fortio:
        ResdataKW.fread(fortio)
        ResdataKW.fread(fortio)
        with pytest.raises(ValueError, match="Failed to create ResdataKW instance"):
            ResdataKW.fread(fortio)
