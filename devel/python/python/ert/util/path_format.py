from ert.cwrap import BaseCClass, CWrapper
from ert.util import UTIL_LIB


class PathFormat(BaseCClass):
    def __init__(self):
        raise NotImplementedError("Class can not be instantiated directly!")

    def __str__(self):
        return PathFormat.cNamespace().get_fmt(self)

    def free(self):
        PathFormat.cNamespace().free( self )


cwrapper = CWrapper(UTIL_LIB)
cwrapper.registerObjectType("path_fmt", PathFormat)

PathFormat.cNamespace().free = cwrapper.prototype("void path_fmt_free(path_fmt)")
PathFormat.cNamespace().get_fmt = cwrapper.prototype("char* path_fmt_get_fmt(path_fmt)")