import datetime

from resdata.util.util import CTime

U32 = 2**32
EPOCH = datetime.datetime(1970, 1, 1, 0, 0, 0)


def assert_epoch(value):
    ctime = CTime(value)
    assert ctime.ctime() == 0
    assert ctime.datetime() == EPOCH


class HugeYearDate(datetime.date):
    @property
    def year(self):
        return U32 + 1970


class NegativeHugeYearDate(datetime.date):
    @property
    def year(self):
        return -U32 + 1970


class HugeMonthDate(datetime.date):
    @property
    def month(self):
        return U32 + 1


class HugeDayDate(datetime.date):
    @property
    def day(self):
        return U32 + 1


class HugeYearDateTime(datetime.datetime):
    @property
    def year(self):
        return U32 + 1970


class HugeMonthDateTime(datetime.datetime):
    @property
    def month(self):
        return U32 + 1


class HugeDayDateTime(datetime.datetime):
    @property
    def day(self):
        return U32 + 1


class HugeHourDateTime(datetime.datetime):
    @property
    def hour(self):
        return U32


class HugeMinuteDateTime(datetime.datetime):
    @property
    def minute(self):
        return U32


class HugeSecondDateTime(datetime.datetime):
    @property
    def second(self):
        return U32


def test_date_year_wraps_like_ctypes_int():
    assert_epoch(HugeYearDate(1970, 1, 1))


def test_date_negative_year_wraps_like_ctypes_int():
    assert_epoch(NegativeHugeYearDate(1970, 1, 1))


def test_date_month_wraps_like_ctypes_int():
    assert_epoch(HugeMonthDate(1970, 1, 1))


def test_date_day_wraps_like_ctypes_int():
    assert_epoch(HugeDayDate(1970, 1, 1))


def test_datetime_year_wraps_like_ctypes_int():
    assert_epoch(HugeYearDateTime(1970, 1, 1, 0, 0, 0))


def test_datetime_month_wraps_like_ctypes_int():
    assert_epoch(HugeMonthDateTime(1970, 1, 1, 0, 0, 0))


def test_datetime_day_wraps_like_ctypes_int():
    assert_epoch(HugeDayDateTime(1970, 1, 1, 0, 0, 0))


def test_datetime_hour_wraps_like_ctypes_int():
    assert_epoch(HugeHourDateTime(1970, 1, 1, 0, 0, 0))


def test_datetime_minute_wraps_like_ctypes_int():
    assert_epoch(HugeMinuteDateTime(1970, 1, 1, 0, 0, 0))


def test_datetime_second_wraps_like_ctypes_int():
    assert_epoch(HugeSecondDateTime(1970, 1, 1, 0, 0, 0))
