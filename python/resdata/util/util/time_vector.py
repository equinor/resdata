import datetime
import re

import resdata.util.util._time_vector as _time_vector

from .ctime import CTime
from .vector_template import VectorTemplate


def _ctime_value(value):
    return CTime(value).value()


class TimeVector(VectorTemplate):
    TYPE_NAME = "rd_time_t_vector"
    default_format = "%d"

    @staticmethod
    def _alloc(initial_size, default_value):
        return _time_vector._alloc(initial_size, _ctime_value(default_value))

    def _alloc_copy(self):
        return TimeVector.createPythonObject(_time_vector._alloc_copy(self))

    def _strided_copy(self, start, stop, stride):
        return TimeVector.createPythonObject(
            _time_vector._strided_copy(self, start, stop, stride)
        )

    def _free(self):
        _time_vector._free(self)

    def _iget(self, index):
        return CTime(_time_vector._iget(self, index))

    def _safe_iget(self, index):
        return CTime(_time_vector._safe_iget(self, index))

    def _iset(self, index, value):
        _time_vector._iset(self, index, _ctime_value(value))

    def _size(self):
        return _time_vector._size(self)

    def _append(self, value):
        _time_vector._append(self, _ctime_value(value))

    def _idel_block(self, index, block_size):
        _time_vector._idel_block(self, index, block_size)

    def _idel(self, index):
        _time_vector._idel(self, index)

    def _pop(self):
        return CTime(_time_vector._pop(self))

    def _lshift(self, shift):
        _time_vector._lshift(self, shift)

    def _rshift(self, shift):
        _time_vector._rshift(self, shift)

    def _insert(self, index, value):
        _time_vector._insert(self, index, _ctime_value(value))

    def _fprintf(self, stream, name, fmt):
        _time_vector._fprintf(self, stream, name, fmt)

    def _sort(self):
        _time_vector._sort(self)

    def _rsort(self):
        _time_vector._rsort(self)

    def _reset(self):
        _time_vector._reset(self)

    def _set_read_only(self, read_only):
        _time_vector._set_read_only(self, read_only)

    def _get_read_only(self):
        return _time_vector._get_read_only(self)

    def _get_max(self):
        return CTime(_time_vector._get_max(self))

    def _get_min(self):
        return CTime(_time_vector._get_min(self))

    def _get_max_index(self, reverse):
        return _time_vector._get_max_index(self, reverse)

    def _get_min_index(self, reverse):
        return _time_vector._get_min_index(self, reverse)

    def _shift(self, delta):
        _time_vector._shift(self, _ctime_value(delta))

    def _scale(self, factor):
        _time_vector._scale(self, _ctime_value(factor))

    def _div(self, divisor):
        _time_vector._div(self, _ctime_value(divisor))

    def _inplace_add(self, delta):
        _time_vector._inplace_add(self, delta)

    def _inplace_mul(self, factor):
        _time_vector._inplace_mul(self, factor)

    def _assign(self, value):
        _time_vector._assign(self, _ctime_value(value))

    def _memcpy(self, src):
        _time_vector._memcpy(self, src)

    def _set_default(self, default_value):
        _time_vector._set_default(self, _ctime_value(default_value))

    def _get_default(self):
        return CTime(_time_vector._get_default(self))

    def _element_size(self):
        return _time_vector._element_size(self)

    def _permute(self, permutation_vector):
        _time_vector._permute(self, permutation_vector)

    def _sort_perm(self):
        from .permutation_vector import PermutationVector

        return PermutationVector.createPythonObject(_time_vector._sort_perm(self))

    def _rsort_perm(self):
        from .permutation_vector import PermutationVector

        return PermutationVector.createPythonObject(_time_vector._rsort_perm(self))

    def _contains(self, value):
        return _time_vector._contains(self, _ctime_value(value))

    def _select_unique(self):
        _time_vector._select_unique(self)

    def _element_sum(self):
        return CTime(_time_vector._element_sum(self))

    def _count_equal(self, value):
        return _time_vector._count_equal(self, _ctime_value(value))

    def _init_range(self, min_value, max_value, delta):
        _time_vector._init_range(
            self, _ctime_value(min_value), _ctime_value(max_value), _ctime_value(delta)
        )

    def _init_linear(self, start_value, end_value, num_values):
        return _time_vector._init_linear(
            self, _ctime_value(start_value), _ctime_value(end_value), num_values
        )

    def _equal(self, other):
        return _time_vector._equal(self, other)

    def _first_eq(self, other, offset):
        return _time_vector._first_eq(self, other, offset)

    def _first_neq(self, other, offset):
        return _time_vector._first_neq(self, other, offset)

    def __init__(self, default_value=None, initial_size=0):
        if default_value is None:
            super().__init__(CTime(0), initial_size)
        else:
            try:
                default = CTime(default_value)
            except:
                raise ValueError(
                    "default value invalid - must be type ctime() or date/datetime"
                )

            super().__init__(default, initial_size)

    @classmethod
    def parseTimeUnit(cls, deltaString):
        deltaRegexp = re.compile(r"(?P<num>\d*)(?P<unit>[dmy])", re.IGNORECASE)
        matchObj = deltaRegexp.match(deltaString)
        if matchObj:
            try:
                num = int(matchObj.group("num"))
            except:
                num = 1

            timeUnit = matchObj.group("unit").lower()
            return num, timeUnit
        else:
            raise TypeError(
                "The delta string must be on form '1d', '2m', 'Y' for one day, two months or one year respectively"
            )

    def __str__(self):
        """
        Returns string representantion of vector.
        """
        string_list = []
        for d in self:
            string_list.append("%s" % d)

        return str(string_list)

    def append(self, value):
        self._append(CTime(value))

    def __contains__(self, value):
        return self._contains(CTime(value))

    def nextTime(self, num, timeUnit):
        currentTime = self[-1].datetime()
        hour = currentTime.hour
        minute = currentTime.minute
        second = currentTime.second

        if timeUnit == "d":
            td = datetime.timedelta(days=num)
            currentTime += td
        else:
            day = currentTime.day
            month = currentTime.month
            year = currentTime.year

            if timeUnit == "y":
                year += num
            else:
                month += num - 1
                deltaYear, newMonth = divmod(month, 12)
                month = newMonth + 1
                year += deltaYear
            currentTime = datetime.datetime(year, month, day, hour, minute, second)

        return currentTime

    def appendTime(self, num, timeUnit):
        next_time = self.nextTime(num, timeUnit)
        self.append(CTime(next_time))

    @classmethod
    def createRegular(cls, start, end, deltaString):
        """
        The last element in the vector will be <= end; i.e. if the
        question of whether the range is closed in the upper end
        depends on the commensurability of the [start,end] interval
        and the delta:

        createRegular(0 , 10 , delta=3) => [0,3,6,9]
        createRegular(0 , 10 , delta=2) => [0,2,4,6,8,10]
        """
        start = CTime(start)
        end = CTime(end)
        if start > end:
            raise ValueError("The time interval is invalid start is after end")

        num, timeUnit = cls.parseTimeUnit(deltaString)

        timeVector = TimeVector()
        currentTime = start
        while currentTime <= end:
            ct = CTime(currentTime)
            timeVector.append(ct)
            currentTime = timeVector.nextTime(num, timeUnit)

        return timeVector

    def getDataPtr(self):
        raise NotImplementedError(
            "The getDataPtr() function is not implemented for time_t vectors"
        )
