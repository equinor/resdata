from cwrap import BaseCClass
from resdata import ResdataPrototype
from resdata.well import WellState


class WellTimeLine(BaseCClass):
    TYPE_NAME = "rd_well_time_line"
    _size = ResdataPrototype("int well_ts_get_size(rd_well_time_line)")
    _name = ResdataPrototype("char* well_ts_get_name(rd_well_time_line)")
    _iget = ResdataPrototype(
        "rd_well_state_ref well_ts_iget_state(rd_well_time_line, int)"
    )

    def __init__(self):
        raise NotImplementedError("Class can not be instantiated directly")

    def getName(self):
        return self._name()

    def __len__(self):
        """@rtype: int"""
        return self._size()

    def __getitem__(self, index):
        """
        @type index: int
        @rtype: WellState
        """

        if index < 0:
            index += len(self)

        if not 0 <= index < len(self):
            raise IndexError("Index must be in range 0 <= %d < %d" % (index, len(self)))

        return self._iget(index).setParent(self)

    def free(self):
        pass

    def __repr__(self):
        n = self.getName()
        l = len(self)
        cnt = "name = %s, size = %d" % (n, l)
        return self._create_repr(cnt)
