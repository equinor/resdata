"""
The resdata package is based on wrapping the libriaries from the C
code with ctypes; an essential part of ctypes approach is to load the
shared libraries with the ctypes.CDLL() function. The ctypes.CDLL()
function uses the standard methods of the operating system,
i.e. standard locations configured with ld.so.conf and the environment
variable LD_LIBRARY_PATH.

To avoid conflict with other application using the ert libraries the
Python code should be able to locate the shared libraries without
(necessarily) using the LD_LIBRARY_PATH variable. The default
behaviour is to try to load from the library ../../lib64, but by using
the enviornment variable RESDATA_LIBRARY_PATH you can alter how ert looks
for shared libraries.

   1. By default the code will try to load the shared libraries from
      '../../lib64' relative to the location of this file.

   2. Depending on the value of RESDATA_LIBRARY_PATH two different
      behaviours can be imposed:

         Existing path: the package will look in the path pointed to
            by RESDATA_LIBRARY_PATH for shared libraries.

         Arbitrary value: the package will use standard load order for
         the operating system.

If the fixed path, given by the default ../../lib64 or RESDATA_LIBRARY_PATH
alternative fails, the loader will try the default load behaviour
before giving up completely.
"""

import ctypes as ct
import os.path
import warnings

warnings.filterwarnings(
    action="always",
    category=DeprecationWarning,
    module="resdata",
)

from cwrap import Prototype

from .version import version as __version__


def _dlopen_resdata():
    import ctypes
    import platform

    path = os.path.join(os.path.dirname(__file__), ".libs")
    if platform.system() == "Linux":
        path = os.path.join(path, "libresdata.so")
    elif platform.system() == "Darwin":
        path = os.path.join(path, "libresdata.dylib")
    elif platform.system() == "Windows":
        path = os.path.join(os.path.dirname(__file__), ".bin", "libresdata.dll")
    else:
        raise NotImplementedError("Invalid platform")

    return ctypes.CDLL(path, ctypes.RTLD_GLOBAL)


# Need to keep the function as a global variable so that we don't give C a
# dangling pointer
_abort_handler = None


@ct.CFUNCTYPE(None, ct.c_char_p, ct.c_int, ct.c_char_p, ct.c_char_p, ct.c_char_p)
def _c_abort_handler(filename, lineno, function, message, backtrace):
    global _abort_handler
    if not _abort_handler:
        return
    _abort_handler(
        filename.decode(),
        lineno,
        function.decode(),
        message.decode(),
        backtrace.decode(),
    )


def set_abort_handler(function):
    """
    Set callback function for util_abort, which is called prior to std::abort()
    """
    global _abort_handler
    _abort_handler = function

    ResdataPrototype.lib.util_set_abort_handler(_c_abort_handler)


class ResdataPrototype(Prototype):
    lib = _dlopen_resdata()

    def __init__(self, prototype, bind=True):
        super(ResdataPrototype, self).__init__(
            ResdataPrototype.lib, prototype, bind=bind
        )


from .rd_type import ResDataType, ResdataTypeEnum
from .rd_util import (
    FileType,
    FileMode,
    Phase,
    UnitSystem,
    ResdataUtil,
)
from .util.util import ResdataVersion, updateAbortSignals

updateAbortSignals()


def root():
    """
    Will print the filesystem root of the current ert package.
    """
    return os.path.abspath(os.path.join(os.path.dirname(__file__), "../"))
