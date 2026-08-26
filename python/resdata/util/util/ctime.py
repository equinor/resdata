from __future__ import annotations

import ctypes
import datetime
import time
from typing import TypeAlias

import numpy as np

import resdata.util.util._ctime as _ctime


class CTime:
    def __init__(self, value: TimeLike):
        if isinstance(value, int):
            self._value = value
        elif isinstance(value, CTime):
            self._value = value.value()
        elif isinstance(value, datetime.datetime):
            self._value = _ctime._timegm(
                value.second,
                value.minute,
                value.hour,
                value.day,
                value.month,
                value.year,
            )
        elif isinstance(value, datetime.date):
            self._value = _ctime._timegm(0, 0, 0, value.day, value.month, value.year)
        elif isinstance(value, np.datetime64):
            d = value.astype("datetime64[s]").item()
            self._value = _ctime._timegm(
                d.second,
                d.minute,
                d.hour,
                d.day,
                d.month,
                d.year,
            )
        else:
            raise NotImplementedError(
                "Can not convert class %s to CTime" % value.__class__
            )

    def value(self):
        return self._value

    def ctime(self) -> int:
        return self.value()

    def time(self):
        """Return this time_t as a time.gmtime() object"""
        return time.gmtime(self.value())

    def date(self) -> datetime.date:
        """Return this time_t as a datetime.date([year, month, day])"""
        return datetime.date(*self.time()[0:3])

    def datetime(self) -> datetime.datetime:
        return datetime.datetime(*self.time()[0:6])

    def __str__(self):
        return self.datetime().strftime("%Y-%m-%d %H:%M:%S%z")

    def __ge__(self, other):
        return self > other or self == other

    def __le__(self, other):
        return self < other or self == other

    def __gt__(self, other):
        if isinstance(other, CTime):
            return self.value() > other.value()
        elif isinstance(other, (int, datetime.datetime, datetime.date, np.datetime64)):
            return self > CTime(other)
        else:
            raise TypeError("CTime does not support type: %s" % other.__class__)

    def __lt__(self, other):
        if isinstance(other, CTime):
            return self.value() < other.value()
        elif isinstance(other, (int, datetime.datetime, datetime.date, np.datetime64)):
            return self < CTime(other)
        else:
            raise TypeError("CTime does not support type: %s" % other.__class__)

    def __ne__(self, other):
        return not self == other

    def __eq__(self, other):
        if isinstance(other, CTime):
            return self.value() == other.value()
        elif isinstance(other, (int, datetime.datetime, datetime.date, np.datetime64)):
            return self == CTime(other)
        elif other is None:
            return False
        else:
            raise TypeError("CTime does not support type: %s" % other.__class__)

    def __imul__(self, other):
        self._value = int(self.value() * other)
        return self

    def __hash__(self):
        return hash(self.value())

    def __iadd__(self, other):
        if isinstance(other, CTime):
            self._value = self.value() + other.value()
            return self
        else:
            self._value = self.value() + CTime(other).value()
            return self

    def __add__(self, other):
        copy = CTime(self)
        copy += other
        return copy

    def __radd__(self, other):
        return self + other

    def __mul__(self, other):
        copy = CTime(self)
        copy *= other
        return copy

    def __rmul__(self, other):
        return self * other

    def timetuple(self):
        # this function is a requirement for comparing against datetime objects where the CTime is on the right side
        pass

    def __repr__(self):
        return "time_t value: %d [%s]" % (self.value(), str(self))

    @property
    def stripped(self):
        return time.strptime(self, "%Y-%m-%d %H:%M:S%")

    @classmethod
    def timezone(cls) -> str:
        """
        Returns the current timezone "in" C
        """
        return _ctime._timezone()


TimeLike: TypeAlias = int | CTime | datetime.datetime | datetime.date | np.datetime64
