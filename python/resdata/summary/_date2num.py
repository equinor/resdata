# The date2num function is a verbatim copy of the _to_ordinalf()
# function from the matplotlib.dates module. Inserted here only to
# avoid importing the full matplotlib library. The date2num
# implementation could be replaced with:
#
#   from matplotlib.dates import date2num
HOURS_PER_DAY = 24.0
MINUTES_PER_DAY = 60 * HOURS_PER_DAY
SECONDS_PER_DAY = 60 * MINUTES_PER_DAY
MUSECONDS_PER_DAY = 1e6 * SECONDS_PER_DAY


def date2num(dt):
    """
    Convert a python datetime instance to UTC float days.

    Convert datetime to the Gregorian date as UTC float days,
    preserving hours, minutes, seconds and microseconds, return value
    is a float. The function is a verbatim copy of the _to_ordinalf()
    function from the matplotlib.dates module.
    """

    if hasattr(dt, "tzinfo") and dt.tzinfo is not None:
        delta = dt.tzinfo.utcoffset(dt)
        if delta is not None:
            dt -= delta

    base = float(dt.toordinal())
    if hasattr(dt, "hour"):
        base += (
            dt.hour / HOURS_PER_DAY
            + dt.minute / MINUTES_PER_DAY
            + dt.second / SECONDS_PER_DAY
            + dt.microsecond / MUSECONDS_PER_DAY
        )
    return base
