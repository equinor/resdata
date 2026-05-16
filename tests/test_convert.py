"""
Tests for the ``convert.x`` command line utility.

``convert.x`` converts reservoir simulator files between formatted (ASCII)
and unformatted (binary) Fortran representations."""

import os
import shutil
import subprocess
from pathlib import Path

import numpy as np
import pytest
import resfo

CONVERT_X = shutil.which("convert.x")


pytestmark = pytest.mark.skipif(
    CONVERT_X is None,
    reason="convert.x binary is not on PATH (only built on Linux)",
)


TEST_DATA = (
    Path(__file__).resolve().parent.parent
    / "test-data"
    / "local"
    / "ECLIPSE"
    / "simple"
)


def run_convert(*args, cwd=None, check=False):
    """Run convert.x with ``args`` and return the CompletedProcess."""
    return subprocess.run(
        [CONVERT_X, *args],
        cwd=cwd,
        capture_output=True,
        text=True,
        check=check,
    )


def assert_equal_fortran_files(path_a, path_b):
    a = resfo.read(str(path_a))
    b = resfo.read(str(path_b))
    assert len(a) == len(b)
    for (kw_a, arr_a), (kw_b, arr_b) in zip(a, b):
        assert kw_a.strip() == kw_b.strip()
        if isinstance(arr_a, np.ndarray) and isinstance(arr_b, np.ndarray):
            assert arr_a.shape == arr_b.shape
            if arr_a.dtype.kind == "f" and arr_b.dtype.kind == "f":
                np.testing.assert_allclose(arr_a, arr_b, rtol=1e-5, atol=1e-7)
            elif arr_a.dtype.kind in ("S", "U") or arr_b.dtype.kind in ("S", "U"):
                # resfo reads Formatted/unformatted files with different
                # numpy dtypes (bytes vs unicode)
                def _norm(x):
                    if isinstance(x, bytes):
                        return x.decode("ascii", errors="replace").strip()
                    return str(x).strip()

                norm_a = [_norm(x) for x in arr_a.tolist()]
                norm_b = [_norm(x) for x in arr_b.tolist()]
                assert norm_a == norm_b
            else:
                np.testing.assert_array_equal(arr_a, arr_b)
        else:
            assert arr_a == arr_b


@pytest.fixture
def simple_unsmry(use_tmpdir):
    for name in ("SIMPLE.UNSMRY", "SIMPLE.SMSPEC"):
        shutil.copy(TEST_DATA / name, name)
    yield Path("SIMPLE.UNSMRY")


def test_no_arguments_prints_usage_and_exits_nonzero():
    result = run_convert()
    assert result.returncode != 0
    assert "Usage: convert.x" in result.stderr


def test_that_deprecation_warning_is_emitted(simple_unsmry):
    result = run_convert("SIMPLE.UNSMRY")
    assert result.returncode == 0
    assert "deprecated" in result.stderr.lower()


def test_that_unsmry_is_converted_to_formatted(simple_unsmry):
    result = run_convert("SIMPLE.UNSMRY")
    assert result.returncode == 0
    assert "Converting SIMPLE.UNSMRY -> SIMPLE.FUNSMRY" in result.stdout
    assert Path("SIMPLE.FUNSMRY").exists()
    assert_equal_fortran_files("SIMPLE.UNSMRY", "SIMPLE.FUNSMRY")


def test_that_funsmry_is_converted_back_to_unformatted(simple_unsmry):
    # First produce the formatted version
    assert run_convert("SIMPLE.UNSMRY").returncode == 0
    os.rename("SIMPLE.UNSMRY", "SIMPLE.UNSMRY.orig")

    result = run_convert("SIMPLE.FUNSMRY")
    assert result.returncode == 0
    assert "Converting SIMPLE.FUNSMRY -> SIMPLE.UNSMRY" in result.stdout
    assert Path("SIMPLE.UNSMRY").exists()
    assert_equal_fortran_files("SIMPLE.UNSMRY.orig", "SIMPLE.UNSMRY")


def test_round_trip_preserves_content(simple_unsmry):
    # UNSMRY -> FUNSMRY
    assert run_convert("SIMPLE.UNSMRY").returncode == 0
    os.rename("SIMPLE.UNSMRY", "ORIG.UNSMRY")
    # FUNSMRY -> UNSMRY
    assert run_convert("SIMPLE.FUNSMRY").returncode == 0
    assert_equal_fortran_files("ORIG.UNSMRY", "SIMPLE.UNSMRY")


def test_that_converting_smspec_roundtrips(simple_unsmry):
    result = run_convert("SIMPLE.SMSPEC")
    assert result.returncode == 0
    assert "Converting SIMPLE.SMSPEC -> SIMPLE.FSMSPEC" in result.stdout
    assert Path("SIMPLE.FSMSPEC").exists()
    assert_equal_fortran_files("SIMPLE.SMSPEC", "SIMPLE.FSMSPEC")


def test_that_converting_unrecognized_file_produces_error_message(simple_unsmry):
    Path("garbage.txt").write_text("not a res file\n")
    result = run_convert("SIMPLE.UNSMRY", "garbage.txt")
    assert result.returncode != 0
    assert "must be recognizable" in result.stderr
    # The first recognizable is still converted
    assert Path("SIMPLE.FUNSMRY").exists()
    assert not Path("garbage.txt.out").exists()


def test_that_restart_file_is_converted_to_formatted(simple_unsmry):
    shutil.copy("SIMPLE.UNSMRY", "SIMPLE.X0042")
    result = run_convert("SIMPLE.X0042")
    assert result.returncode == 0
    assert "Converting SIMPLE.X0042 -> SIMPLE.F0042" in result.stdout
    assert Path("SIMPLE.F0042").exists()
    assert_equal_fortran_files("SIMPLE.X0042", "SIMPLE.F0042")


def test_that_convert_does_not_overwrite_when_target_matches_source(simple_unsmry):
    before = Path("SIMPLE.UNSMRY").read_bytes()
    assert run_convert("SIMPLE.UNSMRY").returncode == 0
    after = Path("SIMPLE.UNSMRY").read_bytes()
    assert before == after
