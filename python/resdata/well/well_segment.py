from cwrap import BaseCClass

import resdata.well._well_segment as _well_segment


class WellSegment(BaseCClass):
    TYPE_NAME = "rd_well_segment"

    def __init__(self):
        raise NotImplementedError("Class can not be instantiated directly")

    def free(self) -> None:
        pass

    def __repr__(self) -> str:
        return "WellSegment(%s) at 0x%x" % (str(self), self._address())

    def __str__(self) -> str:
        return "{Segment ID:%d   BranchID:%d  Length:%g}" % (
            self.id(),
            self.branchId(),
            self.length(),
        )

    def id(self) -> int:
        return _well_segment._id(self)

    def linkCount(self) -> int:
        return _well_segment._link_count(self)

    def branchId(self) -> int:
        return _well_segment._branch_id(self)

    def outletId(self) -> int:
        return _well_segment._outlet_id(self)

    def isActive(self) -> bool:
        return _well_segment._active(self)

    def isMainStem(self) -> bool:
        return _well_segment._main_stem(self)

    def isNearestWellHead(self) -> bool:
        return _well_segment._nearest_wellhead(self)

    def depth(self) -> float:
        return _well_segment._depth(self)

    def __len__(self):
        return self.length()

    def length(self) -> float:
        return _well_segment._length(self)

    def totalLength(self) -> float:
        return _well_segment._total_length(self)

    def diameter(self) -> float:
        return _well_segment._diameter(self)
