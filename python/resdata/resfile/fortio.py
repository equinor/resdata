"""
Module to support transparent binary IO of Fortran created files.

Fortran is a a funny language; when writing binary blobs of data to
file the Fortran runtime will silently add a header and footer around
the date. The Fortran code:

   integer array(100)
   write(unit) array

it actually writes a head and tail in addition to the actual
data. The header and tail is a 4 byte integer, which value is the
number of bytes in the immediately following record. I.e. what is
actually found on disk after the Fortran code above is:

  | 400 | array ...... | 400 |

The fortio.c file implements the fortio_type C structure which can be
used to read and write these structures transparently. The current
python module is a minimal wrapping of this datastructure; mainly to
support passing of FortIO handles to the underlying C functions. A
more extensive wrapping of the fortio implementation would be easy.
"""
import ctypes
import os

from cwrap import BaseCClass
from resdata.util.util import monkey_the_camel
from resdata import ResdataPrototype


class FortIO(BaseCClass):
    TYPE_NAME = "rd_fortio"

    READ_MODE = 1
    WRITE_MODE = 2
    READ_AND_WRITE_MODE = 3
    APPEND_MODE = 4

    _open_reader = ResdataPrototype(
        "void* fortio_open_reader(char*, bool, bool)", bind=False
    )
    _open_writer = ResdataPrototype(
        "void* fortio_open_writer(char*, bool, bool)", bind=False
    )
    _open_readwrite = ResdataPrototype(
        "void* fortio_open_readwrite(char*, bool, bool)", bind=False
    )
    _open_append = ResdataPrototype(
        "void* fortio_open_append(char*, bool, bool)", bind=False
    )
    _guess_fortran = ResdataPrototype(
        "bool fortio_looks_like_fortran_file(char*, bool)", bind=False
    )

    _write_record = ResdataPrototype("void fortio_fwrite_record(rd_fortio, char*, int)")
    _get_position = ResdataPrototype("long fortio_ftell(rd_fortio)")
    _seek = ResdataPrototype("void fortio_fseek(rd_fortio, long, int)")
    _close = ResdataPrototype("bool fortio_fclose(rd_fortio)")
    _truncate = ResdataPrototype("bool fortio_ftruncate(rd_fortio, long)")
    _filename = ResdataPrototype("char* fortio_filename_ref(rd_fortio)")

    def __init__(
        self, file_name, mode=READ_MODE, fmt_file=False, endian_flip_header=True
    ):
        """Will open a new FortIO handle to @file_name - default for reading.

        The newly created FortIO handle will open the underlying FILE*
        for reading, but if you pass the flag mode=FortIO.WRITE_MODE
        the file will be opened for writing.

        Observe that the flag @endian_flip_header will only affect the
        interpretation of the block size markers in the file, endian
        flipping of the actual data blocks must be handled at a higher
        level.

        When you are finished working with the FortIO instance you can
        manually close it with the close() method, alternatively that
        will happen automagically when it goes out of scope.

        Small example script opening a restart file, and then writing
        all the pressure keywords to another file:

           import sys
           from resdata.ecl import FortIO, ResdataFile

           rst_file = ResdataFile(sys.argv[1])
           fortio = FortIO("PRESSURE", mode=FortIO.WRITE_MODE)

           for kw in rst_file:
               if kw.name() == "PRESSURE":
                  kw.write(fortio)

           fortio.close()

        See the documentation of openFortIO() for an alternative
        method based on a context manager and the with statement.

        """
        read_modes = (FortIO.READ_MODE, FortIO.APPEND_MODE, FortIO.READ_AND_WRITE_MODE)
        if mode in read_modes and not os.path.exists(file_name):
            raise IOError('No such file "%s".' % file_name)
        if mode == FortIO.READ_MODE:
            c_pointer = self._open_reader(file_name, fmt_file, endian_flip_header)
        elif mode == FortIO.WRITE_MODE:
            c_pointer = self._open_writer(file_name, fmt_file, endian_flip_header)
        elif mode == FortIO.READ_AND_WRITE_MODE:
            c_pointer = self._open_readwrite(file_name, fmt_file, endian_flip_header)
        elif mode == FortIO.APPEND_MODE:
            c_pointer = self._open_append(file_name, fmt_file, endian_flip_header)
        else:
            raise UserWarning("Unknown mode: %d" % mode)

        self.__mode = mode
        if not c_pointer:
            raise IOError('Failed to open FortIO file "%s".' % file_name)
        super(FortIO, self).__init__(c_pointer)

    def close(self):
        if self:
            self._close()
            self._invalidateCPointer()

    def get_position(self):
        """@rtype: long"""
        return self._get_position()

    def truncate(self, size=None):
        """Will truncate the file to new size.

        If the method is called without a size argument the stream
        will be truncated to the current position.
        """
        if size is None:
            size = self.getPosition()

        if not self._truncate(size):
            raise IOError("Truncate of fortran filehandle:%s failed" % self.filename())

    def filename(self):
        return self._filename()

    def seek(self, position, whence=0):
        # SEEK_SET = 0
        # SEEK_CUR = 1
        # SEEK_END = 2
        self._seek(position, whence)

    @classmethod
    def is_fortran_file(cls, filename, endian_flip=True):
        """@rtype: bool
        @type filename: str


        Will use heuristics to try to guess if @filename is a binary
        file written in fortran style. ASCII files will return false,
        even if they are structured as ECLIPSE keywords.
        """
        return cls._guess_fortran(filename, endian_flip)

    def free(self):
        self.close()


class FortIOContextManager(object):
    def __init__(self, fortio):
        self.__fortio = fortio

    def __enter__(self):
        return self.__fortio

    def __exit__(self, exc_type, exc_val, exc_tb):
        self.__fortio.close()
        return exc_type is not None


def openFortIO(
    file_name, mode=FortIO.READ_MODE, fmt_file=False, endian_flip_header=True
):
    """Will create FortIO based context manager for use with with.

    The with: statement and context managers is a good alternative in
    the situation where you need to ensure resource cleanup.

       import sys
       from resdata.ecl import FortIO, ResdataFile

       rst_file = ResdataFile(sys.argv[1])
       with openFortIO("PRESSURE", mode=FortIO.WRITE_MODE) as fortio:
          for kw in rst_file:
              if kw.name() == "PRESSURE":
                 kw.write(fortio)

    """
    return FortIOContextManager(
        FortIO(
            file_name,
            mode=mode,
            fmt_file=fmt_file,
            endian_flip_header=endian_flip_header,
        )
    )


monkey_the_camel(FortIO, "getPosition", FortIO.get_position)
monkey_the_camel(FortIO, "isFortranFile", FortIO.is_fortran_file, classmethod)
