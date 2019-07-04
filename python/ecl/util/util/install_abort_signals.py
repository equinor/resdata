from ecl import EclPrototype
import os

def installAbortSignals():
    if not os.getenv('ECL_SKIP_SIGNAL'):
        install_signals()


def updateAbortSignals():
    """
    Will install the util_abort_signal for all UNMODIFIED signals.
    """
    if not os.getenv('ECL_SKIP_SIGNAL'):
        update_signals()


install_signals = EclPrototype("void util_install_signals()")
update_signals = EclPrototype("void util_update_signals()")
