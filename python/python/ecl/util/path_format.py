from cwrap import BaseCClass
from ecl.util import UtilPrototype


class PathFormat(BaseCClass):
    TYPE_NAME = "path_fmt"
    _alloc = UtilPrototype("void* path_fmt_alloc_path_fmt(char*)", bind = False)
    _str  = UtilPrototype("char* path_fmt_get_fmt(path_fmt)")
    _free = UtilPrototype("void path_fmt_free(path_fmt)") 
    
    def __init__(self, path_fmt):
        c_ptr = self._alloc( path_fmt )
        super(PathFormat, self).__init__(c_ptr)


    def __str__(self):
        return self._str( )


    def free(self):
        self._free( )
