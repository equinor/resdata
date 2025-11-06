import datetime
from resdata.rft import ResdataRFTFile, ResdataRFTCell
from tests import source_root
import pytest
import os


@pytest.fixture
def spe1_rft_file():
    return os.path.join(
        source_root(), "test-data", "local", "ECLIPSE", "rft", "SPE1.RFT"
    )


def test_RFT_load(spe1_rft_file):
    rft_file = ResdataRFTFile(spe1_rft_file)

    assert rft_file.size() == 218
    assert rft_file.size(well="*") == 218
    assert rft_file.size(well="PROD") == 109
    assert rft_file.size(well="INJ") == 109
    assert rft_file.size(well="INJ", date=datetime.date(2015, 6, 1)) == 1
    assert rft_file.size(date=datetime.date(2015, 6, 1)) == 2
    assert rft_file.size(date=datetime.date(2000, 6, 1)) == 0

    for rft in rft_file:
        assert rft.is_RFT()
        assert not rft.is_SEGMENT()
        assert not rft.is_PLT()
        assert not rft.is_MSW()

        for cell in rft:
            assert isinstance(cell, ResdataRFTCell)
            assert rft.ijkget(cell.get_ijk()) == cell

        assert rft.ijkget((30, 20, 1880)) is None

    for h in rft_file.get_headers():
        assert isinstance(h[1], datetime.date)


def test_exceptions(spe1_rft_file):
    rftFile = ResdataRFTFile(spe1_rft_file)
    with pytest.raises(IndexError):
        _ = rftFile[len(rftFile)]
