from ecl.util.util import setSignalOnUtilAbort
from ecl import EclPrototype
import signal


def test_abort_signal():
    called = {}
    def set_called(a, b):
        called["called"] = True
    signal.signal(signal.SIGUSR1, set_called)
    setSignalOnUtilAbort(signal.SIGUSR1)
    try:
        abort = EclPrototype("void util_abort__(char*, char*, int, char*)")
        abort("", "", 0, "")
        assert "called" in called
    finally:
        setSignalOnUtilAbort(signal.SIGABRT)
