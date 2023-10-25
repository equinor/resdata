"""
The eclfile package contains several classes for working directly with ECLIPSE
files.

  fortio/FortIO: This is functionality to read and write binary
     fortran files.

  ecl_kw/EclKW: This class holds one ECLIPSE keyword, like SWAT, in
     restart format.

  ecl_type/EclDataType: This class is used to represent the data type
    of the elements in EclKW.

  ecl_file/EclFile: This class is used to load an ECLIPSE file in
     restart format, alternatively only parts of the file can be
     loaded. Internally it consists of a collection of EclKW
     instances.
"""

import ecl.util.util

from .fortio import FortIO, openFortIO
from .ecl_kw import EclKW
from .ecl_file_view import EclFileView
from .ecl_file import EclFile, openEclFile
from .ecl_3dkw import Ecl3DKW
from .ecl_3d_file import Ecl3DFile
from .ecl_init_file import EclInitFile
from .ecl_restart_file import EclRestartFile
