import ctypes
import resdata

from resdata import ResdataPrototype
from resdata import ResDataType
from resdata.resfile import ResdataKW, ResdataFile, FortIO
from resdata.util.test import TestAreaContext
from tests import ResdataTest
from resdata.util.util import IntVector


class ResdataIndexedReadTest(ResdataTest):
    _freadIndexedData = ResdataPrototype(
        "void rd_kw_fread_indexed_data_python(rd_fortio, int, rd_data_type, int, rd_int_vector, char*)",
        bind=False,
    )  # fortio, offset, type, count, index_map, buffer
    _resFileIndexedRead = ResdataPrototype(
        "void rd_file_indexed_read(rd_file, char*, int, rd_int_vector, char*)",
        bind=False,
    )  # rd_file, kw, index, index_map, buffer

    def test_rd_kw_indexed_read(self):
        with TestAreaContext("rd_kw_indexed_read") as area:
            fortio = FortIO("index_test", mode=FortIO.WRITE_MODE)

            element_count = 100000
            rd_kw = ResdataKW("TEST", element_count, ResDataType.RD_INT)

            for index in range(element_count):
                rd_kw[index] = index

            rd_kw.fwrite(fortio)

            fortio.close()

            fortio = FortIO("index_test", mode=FortIO.READ_MODE)

            new_rd_kw = ResdataKW.fread(fortio)

            for index in range(element_count):
                self.assertEqual(new_rd_kw[index], index)

            index_map = IntVector()
            index_map.append(2)
            index_map.append(3)
            index_map.append(5)
            index_map.append(7)
            index_map.append(11)
            index_map.append(13)
            index_map.append(313)
            index_map.append(1867)
            index_map.append(5227)
            index_map.append(7159)
            index_map.append(12689)
            index_map.append(18719)
            index_map.append(32321)
            index_map.append(37879)
            index_map.append(54167)
            index_map.append(77213)
            index_map.append(88843)
            index_map.append(99991)

            char_buffer = ctypes.create_string_buffer(
                len(index_map) * ctypes.sizeof(ctypes.c_int)
            )

            self._freadIndexedData(
                fortio,
                24,
                ResDataType.RD_INT,
                element_count,
                index_map,
                char_buffer,
            )

            int_buffer = ctypes.cast(char_buffer, ctypes.POINTER(ctypes.c_int))

            for index, index_map_value in enumerate(index_map):
                self.assertEqual(index_map_value, int_buffer[index])

    def test_rd_file_indexed_read(self):
        with TestAreaContext("rd_file_indexed_read") as area:
            fortio = FortIO("rd_file_index_test", mode=FortIO.WRITE_MODE)

            element_count = 100000
            rd_kw_1 = ResdataKW("TEST1", element_count, ResDataType.RD_INT)
            rd_kw_2 = ResdataKW("TEST2", element_count, ResDataType.RD_INT)

            for index in range(element_count):
                rd_kw_1[index] = index
                rd_kw_2[index] = index + 3

            rd_kw_1.fwrite(fortio)
            rd_kw_2.fwrite(fortio)

            fortio.close()

            rd_file = ResdataFile("rd_file_index_test")

            index_map = IntVector()
            index_map.append(2)
            index_map.append(3)
            index_map.append(5)
            index_map.append(7)
            index_map.append(11)
            index_map.append(13)
            index_map.append(313)
            index_map.append(1867)
            index_map.append(5227)
            index_map.append(7159)
            index_map.append(12689)
            index_map.append(18719)
            index_map.append(32321)
            index_map.append(37879)
            index_map.append(54167)
            index_map.append(77213)
            index_map.append(88843)
            index_map.append(99991)

            char_buffer_1 = ctypes.create_string_buffer(
                len(index_map) * ctypes.sizeof(ctypes.c_int)
            )
            char_buffer_2 = ctypes.create_string_buffer(
                len(index_map) * ctypes.sizeof(ctypes.c_int)
            )

            self._resFileIndexedRead(rd_file, "TEST2", 0, index_map, char_buffer_2)
            self._resFileIndexedRead(rd_file, "TEST1", 0, index_map, char_buffer_1)

            int_buffer_1 = ctypes.cast(char_buffer_1, ctypes.POINTER(ctypes.c_int))
            int_buffer_2 = ctypes.cast(char_buffer_2, ctypes.POINTER(ctypes.c_int))

            for index, index_map_value in enumerate(index_map):
                self.assertEqual(index_map_value, int_buffer_1[index])
                self.assertEqual(index_map_value, int_buffer_2[index] - 3)
