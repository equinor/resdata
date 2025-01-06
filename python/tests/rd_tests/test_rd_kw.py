#!/usr/bin/env python

import pytest

import random
import warnings
import cwrap
import random

from resdata import ResDataType, ResdataTypeEnum, FileMode
from resdata.grid import ResdataRegion, GridGenerator
from resdata.resfile import ResdataKW, ResdataFile, FortIO, openFortIO


from resdata.util.test import TestAreaContext
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

        self.assertEqual(10, kw.getMax())
        self.assertEqual(0, kw.getMin())
        self.assertEqual((0, 10), kw.getMinMax())

    def test_deprecated_datatypes(self):
        with warnings.catch_warnings(record=True) as w:
            warnings.simplefilter("always")
            kw = ResdataKW("Test", 10, ResdataTypeEnum.RD_INT_TYPE)
            self.assertTrue(len(w) > 0)
            self.assertTrue(issubclass(w[-1].category, DeprecationWarning))

        with warnings.catch_warnings(record=True) as w:
            warnings.simplefilter("always")
            kw = ResdataKW("Test", 10, ResDataType.RD_INT)
            self.assertTrue(len(w) == 0)

            self.assertEqual(ResdataTypeEnum.RD_INT_TYPE, kw.type)

            self.assertTrue(len(w) == 1)

    def kw_test(self, data_type, data, fmt):
        name1 = "file1.txt"
        name2 = "file2.txt"
        kw = ResdataKW("TEST", len(data), data_type)
        for i, d in enumerate(data):
            kw[i] = d

        file1 = cwrap.open(name1, "w")
        kw.fprintf_data(file1, fmt)
        file1.close()

        file2 = open(name2, "w")
        for d in data:
            file2.write(fmt % d)
        file2.close()
        self.assertFilesAreEqual(name1, name2)
        self.assertEqual(kw.data_type, data_type)

    def test_create(self):
        with self.assertRaises(ValueError):
            ResdataKW("ThisIsTooLong", 100, ResDataType.RD_CHAR)

    def test_sum(self):
        for rd_type in [ResDataType.RD_CHAR, ResDataType.RD_STRING(42)]:
            kw_string = ResdataKW("STRING", 100, rd_type)
            with self.assertRaises(ValueError):
                kw_string.sum()

        kw_int = ResdataKW("INT", 4, ResDataType.RD_INT)
        kw_int[0] = 1
        kw_int[1] = 2
        kw_int[2] = 3
        kw_int[3] = 4
        self.assertEqual(kw_int.sum(), 10)

        kw_d = ResdataKW("D", 4, ResDataType.RD_DOUBLE)
        kw_d[0] = 1
        kw_d[1] = 2
        kw_d[2] = 3
        kw_d[3] = 4
        self.assertEqual(kw_d.sum(), 10)

        kw_f = ResdataKW("F", 4, ResDataType.RD_FLOAT)
        kw_f[0] = 1
        kw_f[1] = 2
        kw_f[2] = 3
        kw_f[3] = 4
        self.assertEqual(kw_f.sum(), 10)

        kw_b = ResdataKW("F", 4, ResDataType.RD_BOOL)
        kw_b[0] = False
        kw_b[1] = True
        kw_b[2] = False
        kw_b[3] = True
        self.assertEqual(kw_b.sum(), 2)

    def test_fprintf(self):
        with TestAreaContext("python.rd_kw"):
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
        with TestAreaContext("python/rd_kw/writing"):
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
        with TestAreaContext("kw_no_header"):
            kw = ResdataKW("REGIONS", 10, ResDataType.RD_INT)
            for i in range(len(kw)):
                kw[i] = i

            fileH = cwrap.open("test", "w")
            kw.fprintf_data(fileH)
            fileH.close()

            fileH = open("test", "r")
            data = []
            for line in fileH.readlines():
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

        with TestAreaContext("rd_kw/fmt") as ta:
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
            ResdataKW.firstDifferent(kw1, kw2, offset=100)

        with self.assertRaises(ValueError):
            ResdataKW.firstDifferent(kw1, kw3)

        with self.assertRaises(TypeError):
            ResdataKW.firstDifferent(kw1, kw4)

        with self.assertRaises(IndexError):
            kw1.firstDifferent(kw2, offset=100)

        with self.assertRaises(ValueError):
            kw1.firstDifferent(kw3)

        with self.assertRaises(TypeError):
            kw1.firstDifferent(kw4)

        kw1.assign(1)
        kw2.assign(1)

        self.assertEqual(kw1.firstDifferent(kw2), len(kw1))

        kw1[0] = 100
        self.assertEqual(kw1.firstDifferent(kw2), 0)
        self.assertEqual(kw1.firstDifferent(kw2, offset=1), len(kw1))
        kw1[10] = 100
        self.assertEqual(kw1.firstDifferent(kw2, offset=1), 10)

        kw4.assign(1.0)
        kw5.assign(1.0)
        self.assertEqual(kw4.firstDifferent(kw5), len(kw4))

        kw4[10] *= 1.0001
        self.assertEqual(kw4.firstDifferent(kw5), 10)

        self.assertEqual(kw4.firstDifferent(kw5, epsilon=1.0), len(kw4))
        self.assertEqual(kw4.firstDifferent(kw5, epsilon=0.0000001), 10)

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

        view = kw1.numpyView()
        copy = kw1.numpyCopy()
        kw2 = kw1.sub_copy(1, 3)
        self.assertEqual(len(kw2), 3)

        self.assertTrue(copy[0] == kw1[0])
        self.assertTrue(view[0] == kw1[0])

        kw1[0] += 1
        self.assertTrue(view[0] == kw1[0])
        self.assertTrue(copy[0] == kw1[0] - 1)

        for rd_type in [
            ResDataType.RD_CHAR,
            ResDataType.RD_BOOL,
            ResDataType.RD_STRING(19),
        ]:
            kw2 = ResdataKW("TEST_KW", 10, rd_type)
            with self.assertRaises(ValueError):
                kw2.numpyView()

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

        self.assertEqual(kw.typeName(), "INTE")

    def test_string_alloc(self):
        kw = ResdataKW("KW", 10, ResDataType.RD_STRING(30))

        for i in range(10):
            kw[i] = str(i) * 30

        for i in range(10):
            self.assertEqual(str(i) * 30, kw[i])

    def test_string_write_read_unformatted(self):
        for str_len in range(1000):
            with TestAreaContext("my_space"):
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
            with TestAreaContext("my_space"):
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
        with TestAreaContext("FMU_FILES"):
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


def test_get_ptr_data():
    assert ResdataKW("KW1", 10, ResDataType.RD_INT).get_data_ptr()
    assert ResdataKW("KW1", 10, ResDataType.RD_FLOAT).get_data_ptr()
    assert ResdataKW("KW1", 10, ResDataType.RD_DOUBLE).get_data_ptr()
