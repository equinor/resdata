import gc
import sys

import pytest
from resdata import ResDataType
from resdata.resfile import ResdataKW


def test_that_a_failed_construction_does_not_raise_in_del(monkeypatch):
    """If __init__ raises before the pointer/parent/is_reference attributes
    are set, __del__ must not itself raise (e.g. AttributeError) when the
    half-built object is garbage collected."""
    unraisable = []
    monkeypatch.setattr(sys, "unraisablehook", unraisable.append)

    # ResdataKW.__init__ raises ValueError for names longer than 8
    # characters
    with pytest.raises(ValueError, match="maximum eight characters"):
        ResdataKW("TOO_LONG_NAME", 3, ResDataType.RD_INT)

    # Ensure the half-constructed object (if still referenced anywhere,
    # e.g. via the exception traceback) is actually collected now.
    gc.collect()

    assert not unraisable, (
        "__del__ raised an exception on a partially constructed object: "
        f"{[str(u.exc_value) for u in unraisable]}"
    )
