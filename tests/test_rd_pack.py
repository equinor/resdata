"""
Tests for the ``rd_pack.x`` command line utility.

``rd_pack.x`` is the inverse of ``rd_unpack.x``: it combines individual
per-report-step restart (``Xnnnn``/``Fnnnn``) or summary (``Snnnn``/
``Annnn``) files into a single unified file (``UNRST``/``FUNRST`` or
``UNSMRY``/``FUNSMRY``).
"""

import os
import shutil
import subprocess
from pathlib import Path

import numpy as np
import pytest
import resfo

from .test_rd_unpack import run_unpack

RD_PACK_X = shutil.which("rd_pack.x")
RD_UNPACK_X = shutil.which("rd_unpack.x")


pytestmark = pytest.mark.skipif(
    RD_PACK_X is None,
    reason="rd_pack.x binary is not on PATH (only built on Linux)",
)


TEST_DATA = (
    Path(__file__).resolve().parent.parent
    / "test-data"
    / "local"
    / "ECLIPSE"
    / "simple"
)


def run_pack(*args, cwd=None):
    return subprocess.run(
        [RD_PACK_X, *args],
        cwd=cwd,
        capture_output=True,
        text=True,
    )


def _normalize_kw(value):
    if isinstance(value, bytes):
        return value.decode("ascii", errors="replace").strip()
    return str(value).strip()


def _read_kws(path):
    return [(_normalize_kw(kw), arr) for kw, arr in resfo.read(str(path))]


def _assert_arrays_equal(arr_a, arr_b):
    if isinstance(arr_a, np.ndarray) and isinstance(arr_b, np.ndarray):
        assert arr_a.shape == arr_b.shape
        if arr_a.dtype.kind == "f" and arr_b.dtype.kind == "f":
            np.testing.assert_allclose(arr_a, arr_b, rtol=1e-5, atol=1e-7)
        elif arr_a.dtype.kind in ("S", "U") or arr_b.dtype.kind in ("S", "U"):
            assert [_normalize_kw(x) for x in arr_a.tolist()] == [
                _normalize_kw(x) for x in arr_b.tolist()
            ]
        else:
            np.testing.assert_array_equal(arr_a, arr_b)
    else:
        assert arr_a == arr_b


def _assert_streams_equal(path_a, path_b):
    a = _read_kws(path_a)
    b = _read_kws(path_b)
    assert [kw for kw, _ in a] == [kw for kw, _ in b]
    for (_, x), (_, y) in zip(a, b):
        _assert_arrays_equal(x, y)


@pytest.fixture
def simple_unified(use_tmpdir):
    for name in ("SIMPLE.UNRST", "SIMPLE.UNSMRY", "SIMPLE.SMSPEC"):
        shutil.copy(TEST_DATA / name, name)
    yield Path(".")


@pytest.fixture
def split_restart(simple_unified):
    """Unpack SIMPLE.UNRST into Xnnnn files and remove the unified source."""
    assert run_unpack("SIMPLE.UNRST").returncode == 0
    # Keep a pristine copy for round-trip comparisons.
    shutil.copy("SIMPLE.UNRST", "ORIG.UNRST")
    os.remove("SIMPLE.UNRST")
    return sorted(str(p) for p in Path(".").glob("SIMPLE.X[0-9]*"))


@pytest.fixture
def split_summary(simple_unified):
    """Unpack SIMPLE.UNSMRY into Snnnn files and remove the unified source."""
    assert run_unpack("SIMPLE.UNSMRY").returncode == 0
    shutil.copy("SIMPLE.UNSMRY", "ORIG.UNSMRY")
    os.remove("SIMPLE.UNSMRY")
    return sorted(str(p) for p in Path(".").glob("SIMPLE.S[0-9]*"))


def test_no_arguments_is_a_silent_noop():
    result = run_pack()
    assert result.returncode == 0
    assert result.stdout == ""
    assert result.stderr == ""


def test_pack_is_inverse_of_unpack_restart(split_restart):
    """Packing the files produced by rd_unpack.x must yield a unified
    restart whose keyword/array stream equals the original."""
    assert run_pack(*split_restart).returncode == 0
    _assert_streams_equal("ORIG.UNRST", "SIMPLE.UNRST")


def test_pack_is_inverse_of_unpack_summary(split_summary):
    assert run_pack(*split_summary).returncode == 0
    _assert_streams_equal("ORIG.UNSMRY", "SIMPLE.UNSMRY")


def test_pack_restart_inserts_seqnum(split_restart):
    """The unified restart must contain a SEQNUM block separator for each
    input file, with values matching the file's report step."""
    assert run_pack(*split_restart).returncode == 0
    seqnums = [int(arr[0]) for kw, arr in _read_kws("SIMPLE.UNRST") if kw == "SEQNUM"]
    assert seqnums == [1, 2, 3, 4]


def test_pack_summary_has_no_seqnum(split_summary):
    """Unified summary files use SEQHDR (not SEQNUM) as block separator."""
    assert run_pack(*split_summary).returncode == 0
    kws = [kw for kw, _ in _read_kws("SIMPLE.UNSMRY")]
    assert "SEQNUM" not in kws


def test_pack_sorts_files_by_report_step(split_restart):
    shuffled = [split_restart[2], split_restart[0], split_restart[3], split_restart[1]]
    assert run_pack(*shuffled).returncode == 0
    seqnums = [int(arr[0]) for kw, arr in _read_kws("SIMPLE.UNRST") if kw == "SEQNUM"]
    assert seqnums == [1, 2, 3, 4]


def test_pack_duplicate_report_step_errors(split_restart):
    result = run_pack(split_restart[0], split_restart[0])
    assert result.returncode != 0
    assert "same report step twice" in result.stderr


def test_pack_first_arg_smspec_errors(simple_unified):
    result = run_pack("SIMPLE.SMSPEC")
    assert result.returncode != 0
    assert (
        "restart files or summary files" in result.stderr
        or "restart files or summary files" in result.stdout
    )


def test_pack_skips_files_of_other_type(split_restart, simple_unified):
    """The first argument determines the target type; subsequent files of
    a different type are silently skipped."""
    result = run_pack(*split_restart, "SIMPLE.SMSPEC")
    assert result.returncode == 0
    assert Path("SIMPLE.UNRST").exists()
    kws = [kw for kw, _ in _read_kws("SIMPLE.UNRST")]
    assert "INTEHEAD" in kws
    assert "STARTDAT" not in kws  # SMSPEC keyword is not in


def test_pack_formatted_round_trip(simple_unified):
    data = resfo.read("SIMPLE.UNRST")
    resfo.write("SIMPLE.FUNRST", data, fileformat=resfo.Format.FORMATTED)
    assert run_unpack("SIMPLE.FUNRST").returncode == 0
    files = sorted(str(p) for p in Path(".").glob("SIMPLE.F[0-9]*"))
    assert files

    os.remove("SIMPLE.FUNRST")
    assert run_pack(*files).returncode == 0  # writes new SIMPLE.FUNRST
    _assert_streams_equal("SIMPLE.UNRST", "SIMPLE.FUNRST")


def test_pack_writes_to_cwd_not_source_dir(split_restart):
    Path("sub").mkdir()
    moved = []
    for f in split_restart:
        dst = os.path.join("sub", os.path.basename(f))
        shutil.move(f, dst)
        moved.append(dst)
    result = run_pack(*moved)
    assert result.returncode == 0
    assert Path("SIMPLE.UNRST").exists()
    assert not Path("sub", "SIMPLE.UNRST").exists()
