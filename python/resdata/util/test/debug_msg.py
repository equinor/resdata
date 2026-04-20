import inspect
from typing_extensions import deprecated


@deprecated("debug_msg is deprecated and will be removed in version 7")
def debug_msg(msg):
    record = inspect.stack()[1]
    frame = record[0]
    info = inspect.getframeinfo(frame)
    return "FILE: %s  LINE: %s  Msg: %s" % (info.filename, info.lineno, msg)
