import atheris

with atheris.instrument_imports():
    from resdata.summary import Summary

import sys
import tempfile
from pathlib import Path


@atheris.instrument_func
def fuzz_smry(buffer: bytes) -> None:
    smry = None

    sep = b"\x00\x00\x00\x10SEQHDR"
    if sep not in buffer:
        return
    smspec_buffer, unsmry_buffer = buffer.split(sep, maxsplit=1)

    with tempfile.TemporaryDirectory() as tmpdir:
        tmp_path = Path(tmpdir)
        (tmp_path / "TEST.SMSPEC").write_bytes(smspec_buffer)
        (tmp_path / "TEST.UNSMRY").write_bytes(sep + unsmry_buffer)
        try:
            smry = Summary(str(tmp_path / "TEST"))
        except Exception:
            return


if __name__ == "__main__":
    atheris.Setup(
        sys.argv,
        fuzz_smry,
    )
    atheris.Fuzz()
