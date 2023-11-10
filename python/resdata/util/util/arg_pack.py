from cwrap import BaseCClass
from resdata import ResdataPrototype


class ArgPack(BaseCClass):
    TYPE_NAME = "rd_arg_pack"

    _alloc = ResdataPrototype("void* arg_pack_alloc()", bind=False)
    _append_int = ResdataPrototype("void arg_pack_append_int(rd_arg_pack, int)")
    _append_double = ResdataPrototype(
        "void arg_pack_append_double(rd_arg_pack, double)"
    )
    _append_ptr = ResdataPrototype("void arg_pack_append_ptr(rd_arg_pack, void*)")

    _size = ResdataPrototype("int arg_pack_size(rd_arg_pack)")
    _free = ResdataPrototype("void arg_pack_free(rd_arg_pack)")

    def __init__(self, *args):
        c_ptr = self._alloc()
        super(ArgPack, self).__init__(c_ptr)
        self.child_list = []
        for arg in args:
            self.append(arg)

    def append(self, data):
        if isinstance(data, int):
            self._append_int(data)
        elif isinstance(data, float):
            self._append_double(data)
        elif isinstance(data, BaseCClass):
            self._append_ptr(BaseCClass.from_param(data))
            self.child_list.append(data)
        else:
            raise TypeError("Can only add int/double/basecclass")

    def __len__(self):
        return self._size()

    def free(self):
        self._free()
