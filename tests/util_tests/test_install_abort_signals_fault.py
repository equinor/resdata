import importlib
import importlib.abc
import inspect
import sys

import pytest

PUBLIC_MODULE = "resdata.util.util.install_abort_signals"
PUBLIC_PACKAGE = "resdata.util.util"


class RejectUtilChildImports(importlib.abc.MetaPathFinder):
    def find_spec(self, fullname, path, target=None):
        if fullname.startswith(PUBLIC_PACKAGE + ".") and fullname != PUBLIC_MODULE:
            raise ModuleNotFoundError(fullname)


def load_public_module():
    return importlib.import_module(PUBLIC_MODULE)


def public_functions():
    module = load_public_module()
    return module.installAbortSignals, module.updateAbortSignals


def test_public_module_import_does_not_require_helper_modules():
    sys.modules.pop(PUBLIC_MODULE, None)
    finder = RejectUtilChildImports()
    sys.meta_path.insert(0, finder)
    try:
        module = load_public_module()
    finally:
        sys.meta_path.remove(finder)

    assert callable(module.installAbortSignals)
    assert callable(module.updateAbortSignals)


def test_install_abort_signals_returns_none():
    install_abort_signals, _ = public_functions()

    assert install_abort_signals() is None


def test_update_abort_signals_returns_none():
    _, update_abort_signals = public_functions()

    assert update_abort_signals() is None


def test_install_abort_signals_is_idempotent():
    install_abort_signals, _ = public_functions()

    assert [install_abort_signals() for _ in range(5)] == [None] * 5


def test_update_abort_signals_is_idempotent():
    _, update_abort_signals = public_functions()

    assert [update_abort_signals() for _ in range(5)] == [None] * 5


def test_install_and_update_can_be_called_repeatedly_together():
    install_abort_signals, update_abort_signals = public_functions()

    results = []
    for _ in range(3):
        results.append(install_abort_signals())
        results.append(update_abort_signals())

    assert results == [None] * 6


def test_functions_are_exported_from_public_package():
    util = importlib.import_module(PUBLIC_PACKAGE)

    assert callable(util.installAbortSignals)
    assert callable(util.updateAbortSignals)
    assert util.installAbortSignals() is None
    assert util.updateAbortSignals() is None


def test_public_functions_take_no_arguments():
    install_abort_signals, update_abort_signals = public_functions()

    assert list(inspect.signature(install_abort_signals).parameters) == []
    assert list(inspect.signature(update_abort_signals).parameters) == []


def test_install_abort_signals_rejects_arguments():
    install_abort_signals, _ = public_functions()

    with pytest.raises(TypeError, match="takes 0 positional arguments"):
        install_abort_signals(None)


def test_public_functions_reject_unexpected_keywords():
    install_abort_signals, update_abort_signals = public_functions()

    with pytest.raises(TypeError):
        install_abort_signals(unexpected=True)
    with pytest.raises(TypeError):
        update_abort_signals(unexpected=True)
