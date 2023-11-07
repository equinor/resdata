import ctypes
import datetime
import time

from cwrap import BaseCValue
from resdata import ResdataPrototype


class CTime(BaseCValue):
    TYPE_NAME = "time_t"
    DATA_TYPE = ctypes.c_long
    _timezone = ResdataPrototype("char* util_get_timezone()", bind=False)
    _timegm = ResdataPrototype(
        "long util_make_datetime_utc(int, int, int, int, int, int)", bind=False
    )

    def __init__(self, value):
        if isinstance(value, int):
            value = value
        elif isinstance(value, CTime):
            value = value.value()
        elif isinstance(value, datetime.datetime):
            value = CTime._timegm(
                value.second,
                value.minute,
                value.hour,
                value.day,
                value.month,
                value.year,
            )
        elif isinstance(value, datetime.date):
            value = CTime._timegm(0, 0, 0, value.day, value.month, value.year)
        else:
            raise NotImplementedError(
                "Can not convert class %s to CTime" % value.__class__
            )

        super(CTime, self).__init__(value)

    def ctime(self):
        """@rtype: int"""
        return self.value()

    def time(self):
        """Return this time_t as a time.gmtime() object"""
        return time.gmtime(self.value())

    def date(self):
        """Return this time_t as a datetime.date([year, month, day])"""
        return datetime.date(*self.time()[0:3])

    def datetime(self):
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
        elif isinstance(other, (int, datetime.datetime, datetime.date)):
            return self > CTime(other)
        else:
            raise TypeError("CTime does not support type: %s" % other.__class__)

    def __lt__(self, other):
        if isinstance(other, CTime):
            return self.value() < other.value()
        elif isinstance(other, (int, datetime.datetime, datetime.date)):
            return self < CTime(other)
        else:
            raise TypeError("CTime does not support type: %s" % other.__class__)

    def __ne__(self, other):
        return not self == other

    def __eq__(self, other):
        if isinstance(other, CTime):
            return self.value() == other.value()
        elif isinstance(other, (int, datetime.datetime, datetime.date)):
            return self == CTime(other)
        elif isinstance(other, type(None)):
            return False
        else:
            raise TypeError("CTime does not support type: %s" % other.__class__)

    def __imul__(self, other):
        value = int(self.value() * other)
        self.setValue(value)
        return self

    def __hash__(self):
        return hash(self.value())

    def __iadd__(self, other):
        if isinstance(other, CTime):
            self.setValue(self.value() + other.value())
            return self
        else:
            self.setValue(self.value() + CTime(other).value())
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
    def timezone(cls):
        """
        Returns the current timezone "in" C
        @rtype: str
        """
        return CTime._timezone()
