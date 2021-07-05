from ecl import EclPrototype
import sys
import os


def installAbortSignals():
    if sys.version_info.major < 3 and not os.getenv("ECL_SKIP_SIGNAL"):
        install_signals = EclPrototype("void util_install_signals()")
        install_signals()


def updateAbortSignals():
    """
    Will install the util_abort_signal for all UNMODIFIED signals.
    """
    if sys.version_info.major < 3 and not os.getenv("ECL_SKIP_SIGNAL"):
        update_signals = EclPrototype("void util_update_signals()")
        update_signals()
