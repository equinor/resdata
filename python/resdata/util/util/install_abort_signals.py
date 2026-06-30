import os
import sys

import resdata.util.util._install_abort_signals as _install_abort_signals


def installAbortSignals():
    if sys.version_info.major < 3 and not os.getenv("RD_SKIP_SIGNAL"):
        _install_abort_signals._install_signals()


def updateAbortSignals():
    """
    Will install the util_abort_signal for all UNMODIFIED signals.
    """
    if sys.version_info.major < 3 and not os.getenv("RD_SKIP_SIGNAL"):
        _install_abort_signals._update_signals()
