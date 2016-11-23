from cwrap import BaseCClass, CWrapper
from ert.well import ECL_WELL_LIB, WellState

class WellTimeLine(BaseCClass):
    TYPE_NAME = "well_time_line"
    _size = WellPrototype("int well_ts_get_size(well_time_line)")
    _iget = WellPrototype("well_state_ref well_ts_iget_state(well_time_line, int)")

    def __init__(self):
        raise NotImplementedError("Class can not be instantiated directly")


    def __len__(self):
        """ @rtype: int """
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
        return 'WellTimeLine(len = %d) at 0x%x' % (len(self), self._address())
