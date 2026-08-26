import ctypes as ct
import pathlib
import warnings
from importlib.util import find_spec

warnings.filterwarnings(
    action="always",
    category=DeprecationWarning,
    module="resdata",
)

from .rd_type import ResDataType, ResdataTypeEnum
from .rd_util import (
    FileMode,
    FileType,
    Phase,
    ResdataUtil,
    UnitSystem,
)
from .version import version as __version__


def root():
    """
    Will print the filesystem root of the current ert package.
    """
    return os.path.abspath(os.path.join(os.path.dirname(__file__), "../"))
