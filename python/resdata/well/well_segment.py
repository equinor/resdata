from cwrap import BaseCClass
from resdata import ResdataPrototype


class WellSegment(BaseCClass):
    TYPE_NAME = "rd_well_segment"

    _active = ResdataPrototype("bool well_segment_active(rd_well_segment)")
    _main_stem = ResdataPrototype("bool well_segment_main_stem(rd_well_segment)")
    _nearest_wellhead = ResdataPrototype(
        "bool well_segment_nearest_wellhead(rd_well_segment)"
    )
    _id = ResdataPrototype("int well_segment_get_id(rd_well_segment)")
    _link_count = ResdataPrototype("int well_segment_get_link_count(rd_well_segment)")
    _branch_id = ResdataPrototype("int well_segment_get_branch_id(rd_well_segment)")
    _outlet_id = ResdataPrototype("int well_segment_get_outlet_id(rd_well_segment)")
    _depth = ResdataPrototype("double well_segment_get_depth(rd_well_segment)")
    _length = ResdataPrototype("double well_segment_get_length(rd_well_segment)")
    _total_length = ResdataPrototype(
        "double well_segment_get_total_length(rd_well_segment)"
    )
    _diameter = ResdataPrototype("double well_segment_get_diameter(rd_well_segment)")

    def __init__(self):
        raise NotImplementedError("Class can not be instantiated directly")

    def free(self):
        pass

    def __repr__(self):
        return f"WellSegment({str(self)}) at 0x{self._address():x}"

    def __str__(self):
        return "{Segment ID:%d   BranchID:%d  Length:%g}" % (
            self.id(),
            self.branchId(),
            self.length(),
        )

    def id(self):
        """@rtype: int"""
        return self._id()

    def linkCount(self):
        """@rtype: int"""
        return self._link_count()

    def branchId(self):
        """@rtype: int"""
        return self._branch_id()

    def outletId(self):
        """@rtype: int"""
        return self._outlet_id()

    def isActive(self):
        """@rtype: bool"""
        return self._active()

    def isMainStem(self):
        """@rtype: bool"""
        return self._main_stem()

    def isNearestWellHead(self):
        """@rtype: bool"""
        return self._nearest_wellhead()

    def depth(self):
        """@rtype: float"""
        return self._depth()

    def __len__(self):
        return self.length()

    def length(self):
        """@rtype: float"""
        return self._length()

    def totalLength(self):
        """@rtype: float"""
        return self._total_length()

    def diameter(self):
        """@rtype: float"""
        return self._diameter()
