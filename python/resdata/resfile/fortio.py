"""
Module to support transparent binary IO of Fortran created files.

In Fortran, when writing binary blobs of data to file the Fortran runtime will
add a header and footer around the data. The Fortran code:

   integer array(100)
   write(unit) array

writes a head and tail in addition to the actual data. The header and tail are
4-byte integers whose value is the number of bytes in the immediately following
record. I.e. what is actually found on disk after the Fortran code above is:

  | 400 | array ...... | 400 |

The FortIO.cpp file implements the fortio_type struct which can be used to read
and write these. The current Python module is a minimal wrapping of this data
structure; mainly to support passing FortIO handles to the underlying cpp
functions.
"""

from resdata.resfile._fortio import FortIO
from resdata.util.util import monkey_the_camel


class FortIOContextManager:
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
    """Create a FortIO-based context manager for use with the "with" statement.

    The with statement and context managers are a good alternative in
    situations where you need to ensure resource cleanup.

       import sys
       from resdata.resfile import FortIO, openFortIO, ResdataFile

       rst_file = ResdataFile(sys.argv[1])
       with openFortIO("PRESSURE", mode=FortIO.WRITE_MODE) as fortio:
          for kw in rst_file:
              if kw.name() == "PRESSURE":
                 kw.fwrite(fortio)

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

__all__ = ["FortIO", "FortIOContextManager", "openFortIO"]
