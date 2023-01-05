from cwrap import BaseCClass
from ecl import EclPrototype


class ArgPack(BaseCClass):
    TYPE_NAME = "arg_pack"

    _alloc = EclPrototype("void* arg_pack_alloc()", bind=False)
    _append_int = EclPrototype("void arg_pack_append_int(arg_pack, int)")
    _append_double = EclPrototype("void arg_pack_append_double(arg_pack, double)")
    _append_ptr = EclPrototype("void arg_pack_append_ptr(arg_pack, void*)")

    _size = EclPrototype("int arg_pack_size(arg_pack)")
    _free = EclPrototype("void arg_pack_free(arg_pack)")

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
