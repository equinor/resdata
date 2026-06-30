from cwrap import BaseCClass

import resdata.util.util._lookup_table as _lookup_table


class LookupTable(BaseCClass):
    TYPE_NAME = "rd_lookup_table"

    def __init__(self, lower_limit=None, upper_limit=None):
        super().__init__(_lookup_table._alloc())

        if lower_limit is not None:
            self.setLowerLimit(lower_limit)

        if upper_limit is not None:
            self.setUpperLimit(upper_limit)

    def getMaxValue(self):
        self.assertSize(1)
        return _lookup_table._max(self)

    def getMinValue(self):
        self.assertSize(1)
        return _lookup_table._min(self)

    def getMinArg(self):
        self.assertSize(1)
        return _lookup_table._arg_min(self)

    def getMaxArg(self):
        self.assertSize(1)
        return _lookup_table._arg_max(self)

    def assertSize(self, N):
        if len(self) < N:
            raise ValueError("Lookup table is too small")

    def __len__(self):
        return _lookup_table._size(self)

    @property
    def size(self):
        return len(self)

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
        _lookup_table._set_low_limit(self, value)

    def hasLowerLimit(self):
        return _lookup_table._has_low_limit(self)

    def setUpperLimit(self, value):
        _lookup_table._set_high_limit(self, value)

    def hasUpperLimit(self):
        return _lookup_table._has_high_limit(self)

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

        return _lookup_table._interp(self, x)

    def append(self, x, y):
        _lookup_table._append(self, x, y)

    # todo: necessary???
    def __del__(self):
        _lookup_table._free(self)

    def free(self):
        _lookup_table._free(self)
