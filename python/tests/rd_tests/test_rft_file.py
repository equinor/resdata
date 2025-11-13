import datetime
from resdata.rft import ResdataRFTFile, ResdataRFTCell
from tests import source_root
import pytest
import shutil
from pathlib import Path
import numpy as np
import resfo
from functools import partial


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


def well_etc(
    time_units=b"HOURS   ",
    lgr_name=b"        ",
    data_category=b"R        ",
):
    return np.array(
        [
            time_units,
            b"WELL1   ",
            lgr_name,
            b"METRES  ",
            b"BARSA   ",
            data_category,
            b"STANDARD",
            b"SM3/DAY ",
            b"SM3/DAY ",
            b"RM3/DAY ",
            b"        ",
            b"M/SEC   ",
            b"CP      ",
            b"KG/SM3  ",
            b"KG/DAY  ",
            b"KG/KG   ",
        ]
    )


float_arr = partial(np.array, dtype=np.float32)
int_arr = partial(np.array, dtype=np.int32)


def test_rft_data(tmp_path):
    file = tmp_path / "CASE.RFT"
    resfo.write(
        file,
        [
            ("TIME    ", float_arr([24.0])),
            ("DATE    ", int_arr([1, 1, 2000])),
            ("WELLETC ", well_etc()),
            ("CONIPOS ", int_arr([1, 2])),
            ("CONJPOS ", int_arr([1, 1])),
            ("CONKPOS ", int_arr([1, 2])),
            ("PRESSURE", float_arr([100.0, 200.0])),
            ("SWAT    ", float_arr([99.0, 2098.0])),
            ("SGAS    ", float_arr([40.0, 41.0])),
            ("DEPTH   ", float_arr([20.0, 30.0])),
            ("TIME    ", float_arr([34.0])),
            ("DATE    ", int_arr([2, 2, 2002])),
            ("WELLETC ", well_etc()),
            ("CONIPOS ", int_arr([1, 2])),
            ("CONJPOS ", int_arr([1, 1])),
            ("CONKPOS ", int_arr([1, 2])),
            ("PRESSURE", float_arr([101.0, 201.0])),
            ("SWAT    ", float_arr([199.0, 2198.0])),
            ("SGAS    ", float_arr([140.0, 141.0])),
            ("DEPTH   ", float_arr([21.0, 31.0])),
        ],
    )
    rft_file = ResdataRFTFile(str(file))

    assert len(rft_file) == 2
    assert rft_file.size() == 2
    assert rft_file.get_num_wells() == 1

    node = rft_file[0]
    assert node.is_RFT()
    assert not node.is_PLT()
    assert not node.is_SEGMENT()
    assert not node.is_MSW()
    assert len(node) == 2


def test_rft_with_plt_data(tmp_path):
    file = tmp_path / "CASE.RFT"
    resfo.write(
        file,
        [
            ("TIME    ", float_arr([24.0])),
            ("DATE    ", int_arr([1, 1, 2000])),
            ("WELLETC ", well_etc(data_category=b"P       ")),
            ("CONIPOS ", int_arr([1, 2])),
            ("CONJPOS ", int_arr([1, 1])),
            ("CONKPOS ", int_arr([1, 2])),
            ("PRESSURE", float_arr([100.0, 200.0])),
            ("SWAT    ", float_arr([99.0, 2098.0])),
            ("SGAS    ", float_arr([40.0, 41.0])),
            ("DEPTH   ", float_arr([20.0, 30.0])),
            ("CONWRAT ", float_arr([2.0, 3.0])),
            ("CONGRAT ", float_arr([2.0, 3.0])),
            ("CONORAT ", float_arr([2.0, 3.0])),
            ("CONDEPTH", float_arr([2.0, 3.0])),
            ("CONVTUB ", float_arr([2.0, 3.0])),
            ("CONOTUB ", float_arr([2.0, 3.0])),
            ("CONGTUB ", float_arr([2.0, 3.0])),
            ("CONWTUB ", float_arr([2.0, 3.0])),
            ("CONPRES ", float_arr([2.0, 3.0])),
        ],
    )
    rft_file = ResdataRFTFile(str(file))
    node = rft_file[0]
    assert not node.is_RFT()
    assert node.is_PLT()
    assert not node.is_SEGMENT()
    assert not node.is_MSW()


def test_that_rft_node_is_msw_is_based_on_conlenst_kw(tmp_path):
    file = tmp_path / "CASE.RFT"
    resfo.write(
        file,
        [
            ("TIME    ", float_arr([24.0])),
            ("DATE    ", int_arr([1, 1, 2000])),
            ("WELLETC ", well_etc()),
            ("CONIPOS ", int_arr([1, 2])),
            ("CONJPOS ", int_arr([1, 1])),
            ("CONKPOS ", int_arr([1, 2])),
            ("PRESSURE", float_arr([100.0, 200.0])),
            ("SWAT    ", float_arr([99.0, 2098.0])),
            ("SGAS    ", float_arr([40.0, 41.0])),
            ("DEPTH   ", float_arr([20.0, 30.0])),
            ("CONLENST", float_arr([33.0, 34.0])),
            ("TIME    ", float_arr([34.0])),
            ("DATE    ", int_arr([2, 2, 2002])),
            ("WELLETC ", well_etc()),
            ("CONIPOS ", int_arr([1, 2])),
            ("CONJPOS ", int_arr([1, 1])),
            ("CONKPOS ", int_arr([1, 2])),
            ("PRESSURE", float_arr([101.0, 201.0])),
            ("SWAT    ", float_arr([199.0, 2198.0])),
            ("SGAS    ", float_arr([140.0, 141.0])),
            ("DEPTH   ", float_arr([21.0, 31.0])),
        ],
    )
    rft_file = ResdataRFTFile(str(file))

    # Tests that is_MSW() returns True if CONLENST keyword is present in file for backwards compatibility
    assert rft_file[0].is_MSW()
    assert not rft_file[1].is_MSW()
