"""
The resfile package contains several classes for working directly with restart
format files.

  fortio/FortIO: This is functionality to read and write binary
     fortran files.

  rd_kw/ResdataKW: This class holds one keyword, like SWAT, in
     restart format.

  rd_type/ResDataType: This class is used to represent the data type
    of the elements in ResdataKW.

  rd_file/ResdataFile: This class is used to load a file in
     restart format, alternatively only parts of the file can be
     loaded. Internally it consists of a collection of ResdataKW
     instances.
"""

import resdata.util.util

from .fortio import FortIO, openFortIO
from .rd_kw import ResdataKW
from .rd_file_view import ResdataFileView
from .rd_file import ResdataFile, openResdataFile
from .rd_3dkw import Resdata3DKW
from .rd_3d_file import Resdata3DFile
from .rd_init_file import ResdataInitFile
from .rd_restart_file import ResdataRestartFile
