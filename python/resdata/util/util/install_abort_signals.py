from resdata import ResdataPrototype
import sys
import os


def installAbortSignals():
    if sys.version_info.major < 3 and not os.getenv("RD_SKIP_SIGNAL"):
        install_signals = ResdataPrototype("void util_install_signals()")
        install_signals()


def updateAbortSignals():
    """
    Will install the util_abort_signal for all UNMODIFIED signals.
    """
    if sys.version_info.major < 3 and not os.getenv("RD_SKIP_SIGNAL"):
        update_signals = ResdataPrototype("void util_update_signals()")
        update_signals()
