from ert.util import UTIL_LIB
from ert.cwrap import CWrapper

def installAbortSignals():
    install_signals()


def updateAbortSignals():
    """
    Will install the util_abort_signal for all UNMODIFIED signals.
    """
    update_signals()
    
cwrapper = CWrapper(UTIL_LIB)
install_signals = cwrapper.prototype("void util_install_signals()")
update_signals = cwrapper.prototype("void util_update_signals()")
