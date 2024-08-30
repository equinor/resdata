import inspect


def debug_msg(msg):
    record = inspect.stack()[1]
    frame = record[0]
    info = inspect.getframeinfo(frame)
    return f"FILE: {info.filename}  LINE: {info.lineno}  Msg: {msg}"
