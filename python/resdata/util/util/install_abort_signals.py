import os
import sys

from typing_extensions import deprecated

from resdata import ResdataPrototype


@deprecated("installAbortSignals is deprecated and will be removed in version 7.")
def installAbortSignals():
    if sys.version_info.major < 3 and not os.getenv("RD_SKIP_SIGNAL"):
        install_signals = ResdataPrototype("void util_install_signals()")
        install_signals()


@deprecated("updateAbortSignals is deprecated and will be removed in version 7.")
def updateAbortSignals():
    """
    Will install the util_abort_signal for all UNMODIFIED signals.
    """
    if sys.version_info.major < 3 and not os.getenv("RD_SKIP_SIGNAL"):
        update_signals = ResdataPrototype("void util_update_signals()")
        update_signals()
