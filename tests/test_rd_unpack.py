"""
Tests for the ``rd_unpack.x`` command line utility.

``rd_unpack.x`` splits a unified summary (``UNSMRY``/``FUNSMRY``) or unified
restart (``UNRST``/``FUNRST``) file into individual per-report-step files.
Output is always written to the current working directory regardless of
where the source file lives.
"""

import os
import shutil
import subprocess
from pathlib import Path

import numpy as np
import pytest
import resfo

RD_UNPACK_X = shutil.which("rd_unpack.x")


pytestmark = pytest.mark.skipif(
    RD_UNPACK_X is None,
    reason="rd_unpack.x binary is not on PATH (only built on Linux)",
)


TEST_DATA = (
    Path(__file__).resolve().parent.parent
    / "test-data"
    / "local"
    / "ECLIPSE"
    / "simple"
)


def run_unpack(*args, cwd=None):
    return subprocess.run(
        [RD_UNPACK_X, *args],
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


def _assert_concatenation_equals(unified_path, split_paths):
    expected = _read_kws(unified_path)
    actual = []
    for p in split_paths:
        actual.extend(_read_kws(p))
    assert [kw for kw, _ in expected] == [kw for kw, _ in actual]
    for (_, a), (_, b) in zip(expected, actual):
        _assert_arrays_equal(a, b)


@pytest.fixture
def simple_unified(use_tmpdir):
    for name in ("SIMPLE.UNRST", "SIMPLE.UNSMRY", "SIMPLE.SMSPEC"):
        shutil.copy(TEST_DATA / name, name)
    yield Path(".")


def test_no_arguments_exits_nonzero():
    result = run_unpack()
    assert result.returncode != 0
    assert "rd_unpack" in result.stderr or "rd_unpack" in result.stdout


def test_unpack_unified_restart(simple_unified):
    result = run_unpack("SIMPLE.UNRST")
    assert result.returncode == 0
    expected = ["SIMPLE.X0001", "SIMPLE.X0002", "SIMPLE.X0003", "SIMPLE.X0004"]
    for f in expected:
        assert Path(f).exists()


def test_unpack_unified_summary(simple_unified):
    result = run_unpack("SIMPLE.UNSMRY")
    assert result.returncode == 0
    expected = ["SIMPLE.S0001", "SIMPLE.S0002", "SIMPLE.S0003", "SIMPLE.S0004"]
    for f in expected:
        assert Path(f).exists()


def test_unpack_restart_round_trip(simple_unified):
    """Concatenating the unpacked restart files reproduces the unified file
    content (apart from the SEQNUM separator keyword)."""
    assert run_unpack("SIMPLE.UNRST").returncode == 0
    split = sorted(Path(".").glob("SIMPLE.X[0-9]*"))
    unified_kws = _read_kws("SIMPLE.UNRST")
    filtered = [(kw, arr) for kw, arr in unified_kws if kw != "SEQNUM"]
    split_kws = []
    for p in split:
        split_kws.extend(_read_kws(p))
    assert [kw for kw, _ in filtered] == [kw for kw, _ in split_kws]
    for (_, a), (_, b) in zip(filtered, split_kws):
        _assert_arrays_equal(a, b)


def test_unpack_summary_round_trip(simple_unified):
    assert run_unpack("SIMPLE.UNSMRY").returncode == 0
    split = sorted(Path(".").glob("SIMPLE.S[0-9]*"))
    _assert_concatenation_equals("SIMPLE.UNSMRY", split)


def test_unpack_formatted_unified_restart(simple_unified):
    data = resfo.read("SIMPLE.UNRST")
    resfo.write("SIMPLE.FUNRST", data, fileformat=resfo.Format.FORMATTED)
    result = run_unpack("SIMPLE.FUNRST")
    assert result.returncode == 0
    expected = ["SIMPLE.F0001", "SIMPLE.F0002", "SIMPLE.F0003", "SIMPLE.F0004"]
    for f in expected:
        assert Path(f).exists(), f"missing {f}"


def test_unpack_multiple_files(simple_unified):
    result = run_unpack("SIMPLE.UNRST", "SIMPLE.UNSMRY")
    assert result.returncode == 0
    for f in ("SIMPLE.X0001", "SIMPLE.X0004", "SIMPLE.S0001", "SIMPLE.S0004"):
        assert Path(f).exists(), f"missing {f}"


def test_unpack_non_unified_file_aborts(simple_unified):
    result = run_unpack("SIMPLE.SMSPEC")
    assert result.returncode != 0
    assert "unified" in result.stderr.lower() or "unified" in result.stdout.lower()


def test_unpack_unrecognized_extension_aborts(simple_unified):
    Path("garbage.txt").write_text("not a res file\n")
    result = run_unpack("garbage.txt")
    assert result.returncode != 0


def test_unpack_writes_to_cwd_not_source_dir(simple_unified):
    Path("sub").mkdir()
    shutil.move("SIMPLE.UNRST", "sub/SIMPLE.UNRST")
    result = run_unpack(os.path.join("sub", "SIMPLE.UNRST"))
    assert result.returncode == 0
    # Outputs should land in cwd, not in sub/.
    for f in ("SIMPLE.X0001", "SIMPLE.X0002", "SIMPLE.X0003", "SIMPLE.X0004"):
        assert Path(f).exists(), f"missing {f} in cwd"
        assert not Path("sub", f).exists(), f"unexpected {f} in sub/"
