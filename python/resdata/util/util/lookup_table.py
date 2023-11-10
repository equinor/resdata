from cwrap import BaseCClass
from resdata import ResdataPrototype


class LookupTable(BaseCClass):
    TYPE_NAME = "rd_lookup_table"
    _alloc = ResdataPrototype("void* lookup_table_alloc_empty()", bind=False)
    _max = ResdataPrototype("double lookup_table_get_max_value(rd_lookup_table)")
    _min = ResdataPrototype("double lookup_table_get_min_value(rd_lookup_table)")
    _arg_max = ResdataPrototype("double lookup_table_get_max_arg(rd_lookup_table)")
    _arg_min = ResdataPrototype("double lookup_table_get_min_arg(rd_lookup_table)")
    _append = ResdataPrototype(
        "void lookup_table_append(rd_lookup_table, double, double)"
    )
    _size = ResdataPrototype("int lookup_table_get_size(rd_lookup_table)")
    _interp = ResdataPrototype("double lookup_table_interp(rd_lookup_table, double)")
    _free = ResdataPrototype("void lookup_table_free(rd_lookup_table)")
    _set_low_limit = ResdataPrototype(
        "void lookup_table_set_low_limit(rd_lookup_table, double)"
    )
    _set_high_limit = ResdataPrototype(
        "void lookup_table_set_high_limit(rd_lookup_table, double)"
    )
    _has_low_limit = ResdataPrototype(
        "bool lookup_table_has_low_limit(rd_lookup_table)"
    )
    _has_high_limit = ResdataPrototype(
        "bool lookup_table_has_high_limit(rd_lookup_table)"
    )

    def __init__(self, lower_limit=None, upper_limit=None):
        super(LookupTable, self).__init__(self._alloc())

        if not lower_limit is None:
            self.setLowerLimit(lower_limit)

        if not upper_limit is None:
            self.setUpperLimit(upper_limit)

    def getMaxValue(self):
        self.assertSize(1)
        return self._max()

    def getMinValue(self):
        self.assertSize(1)
        return self._min()

    def getMinArg(self):
        self.assertSize(1)
        return self._arg_min()

    def getMaxArg(self):
        self.assertSize(1)
        return self._arg_max()

    def assertSize(self, N):
        if len(self) < N:
            raise ValueError("Lookup table is too small")

    def __len__(self):
        return self._size()

    @property
    def size(self):
        return len(self)

    # Deprecated properties
    @property
    def max(self):
        return self.getMaxValue()

    @property
    def min(self):
        return self.getMinValue()

    @property
    def arg_max(self):
        return self.getMaxArg()

    @property
    def arg_min(self):
        return self.getMinArg()

    def setLowerLimit(self, value):
        self._set_low_limit(value)

    def hasLowerLimit(self):
        return self._has_low_limit()

    def setUpperLimit(self, value):
        self._set_high_limit(value)

    def hasUpperLimit(self):
        return self._has_high_limit()

    def interp(self, x):
        self.assertSize(2)
        if x < self.getMinArg():
            if not self.hasLowerLimit():
                raise ValueError(
                    "Interpolate argument:%g is outside valid interval: [%g,%g]"
                    % (x, self.getMinArg(), self.getMaxArg())
                )
        elif x > self.getMaxArg():
            if not self.hasUpperLimit():
                raise ValueError(
                    "Interpolate argument:%g is outside valid interval: [%g,%g]"
                    % (x, self.getMinArg(), self.getMaxArg())
                )

        return self._interp(x)

    def append(self, x, y):
        self._append(x, y)

    # todo: necessary???
    def __del__(self):
        self._free()

    def free(self):
        self._free()
