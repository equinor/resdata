from contextlib import contextmanager

from resdata.resfile.fortio import FortIO


@contextmanager
def FortIOContextManager(fortio):
    try:
        yield fortio
    finally:
        fortio.close()


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


__all__ = ["FortIOContextManager", "openFortIO"]
