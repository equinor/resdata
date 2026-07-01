from cwrap import BaseCClass

import resdata.well._well_time_line as _well_time_line

from .well_state import WellState


class WellTimeLine(BaseCClass):
    TYPE_NAME = "rd_well_time_line"

    def __init__(self):
        raise NotImplementedError("Class can not be instantiated directly")

    def getName(self) -> str:
        return _well_time_line._name(self)

    def __len__(self) -> int:
        return _well_time_line._size(self)

    def __getitem__(self, index: int) -> WellState:
        if index < 0:
            index += len(self)

        if not 0 <= index < len(self):
            raise IndexError("Index must be in range 0 <= %d < %d" % (index, len(self)))

        return _well_time_line._iget(self, index)

    def free(self) -> None:
        pass

    def __repr__(self) -> str:
        n = self.getName()
        l = len(self)
        cnt = "name = %s, size = %d" % (n, l)
        return self._create_repr(cnt)
