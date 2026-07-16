"""
The resfile package contains several classes for working directly with restart
format files.

  FortIO: This is functionality to read and write binary
     fortran files.

  ResdataKW: This class holds one keyword, like SWAT, in
     restart format.

  ResDataType: This class is used to represent the data type
    of the elements in ResdataKW.

  ResdataFile: This class is used to load a file in
     restart format, alternatively only parts of the file can be
     loaded. Internally it consists of a collection of ResdataKW
     instances.
"""

from ._open_fortio import openFortIO
from ._open_rd_file import open_rd_file
from .fortio import FortIO
from .rd_3d_file import Resdata3DFile
from .rd_3dkw import Resdata3DKW
from .rd_file import ResdataFile
from .rd_file_view import ResdataFileView
from .rd_init_file import ResdataInitFile
from .rd_kw import ResdataKW
from .rd_restart_file import ResdataRestartFile

__all__ = [
    "FortIO",
    "Resdata3DFile",
    "Resdata3DKW",
    "ResdataFile",
    "ResdataFileView",
    "ResdataInitFile",
    "ResdataKW",
    "ResdataRestartFile",
    "openFortIO",
    "open_rd_file",
]
