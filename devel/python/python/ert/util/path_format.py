from ert.cwrap import BaseCClass
from ert.util import BoundUtilPrototype


class PathFormat(BaseCClass):
    TYPE_NAME = "path_fmt"

    def __init__(self):
        raise NotImplementedError("Class can not be instantiated directly!")

    __str__ = BoundUtilPrototype("char* path_fmt_get_fmt(path_fmt)")
    free = BoundUtilPrototype("void path_fmt_free(path_fmt)")
