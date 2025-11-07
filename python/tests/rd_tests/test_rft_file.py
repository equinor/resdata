import datetime
from resdata.rft import ResdataRFTFile, ResdataRFTCell
from tests import source_root
import pytest
import shutil
from pathlib import Path


@pytest.fixture
def spe1_rft_file(tmp_path):
    destination = tmp_path / "SPE1.RFT"
    shutil.copyfile(
        Path(source_root()) / "test-data" / "local" / "ECLIPSE" / "rft" / "SPE1.RFT",
        destination,
    )
    return destination


@pytest.fixture
def spe1_frft_file(tmp_path):
    destination = tmp_path / "SPE1.FRFT"
    shutil.copyfile(
        Path(source_root()) / "test-data" / "local" / "ECLIPSE" / "rft" / "SPE1.FRFT",
        destination,
    )
    return destination


def test_RFT_load(spe1_rft_file):
    rft_file = ResdataRFTFile(str(spe1_rft_file))

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


def test_that_rft_file_will_find_rft_file_by_data_file_name(spe1_rft_file):
    rft_file = ResdataRFTFile(
        str(spe1_rft_file.parent / (str(spe1_rft_file.stem) + ".DATA"))
    )
    assert rft_file.size(well="PROD") == 109


def test_that_rft_file_will_find_frft_file_by_data_file_name(spe1_frft_file):
    rft_file = ResdataRFTFile(
        str(spe1_frft_file.parent / (str(spe1_frft_file.stem) + ".DATA"))
    )
    assert rft_file.size(well="FPROD") == 109


def test_that_rft_file_will_find_rft_file_by_basename(spe1_rft_file):
    rft_file = ResdataRFTFile(str(spe1_rft_file.parent / spe1_rft_file.stem))
    assert rft_file.size(well="PROD") == 109


def test_that_rft_file_will_find_frft_file_by_basename(spe1_frft_file):
    rft_file = ResdataRFTFile(str(spe1_frft_file.parent / spe1_frft_file.stem))
    assert rft_file.size(well="FPROD") == 109


def test_that_finding_by_case_name_chooses_unformatted_first(
    spe1_rft_file, spe1_frft_file
):
    rft_file = ResdataRFTFile(str(spe1_rft_file.parent / spe1_rft_file.stem))
    assert rft_file.size(well="PROD") == 109


def test_that_non_existent_rft_file_raises_valueerror(tmp_path):
    with pytest.raises(ValueError):
        ResdataRFTFile(str(tmp_path / "DOES_NOT_EXIST"))


def test_exceptions(spe1_rft_file):
    rftFile = ResdataRFTFile(str(spe1_rft_file))
    with pytest.raises(IndexError):
        _ = rftFile[len(rftFile)]
