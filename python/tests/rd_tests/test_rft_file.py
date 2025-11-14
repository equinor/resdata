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


def test_that_reading_of_spe1_results_in_expected_values(spe1_rft_file):
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


def test_that_looking_up_indices_beyond_the_size_of_the_rft_raises_indexerror(
    spe1_rft_file,
):
    rftFile = ResdataRFTFile(str(spe1_rft_file))
    with pytest.raises(IndexError):
        _ = rftFile[len(rftFile)]
    with pytest.raises(IndexError):
        _ = rftFile[-1]


def well_etc(
    time_units=b"HOURS",
    lgr_name=b"",
    data_category=b"R",
    well_name=b"WELL1",
):
    return np.array(
        [
            time_units,
            well_name,
            lgr_name,
            b"METRES",
            b"BARSA",
            data_category,
            b"STANDARD",
            b"SM3/DAY",
            b"SM3/DAY",
            b"RM3/DAY",
            b"",
            b"M/SEC",
            b"CP",
            b"KG/SM3",
            b"KG/DAY",
            b"KG/KG",
        ]
    )


float_arr = partial(np.array, dtype=np.float32)
int_arr = partial(np.array, dtype=np.int32)


def cell_start(date=(1, 1, 2000), *args, **kwargs):
    return [
        ("TIME    ", float_arr([24.0])),
        ("DATE    ", int_arr(date)),
        ("WELLETC ", well_etc(*args, **kwargs)),
        ("CONIPOS ", int_arr([1, 2])),
        ("CONJPOS ", int_arr([1, 1])),
        ("CONKPOS ", int_arr([1, 2])),
    ]


def plt_fields():
    return [
        ("CONWRAT ", float_arr([2.0, 3.0])),
        ("CONGRAT ", float_arr([2.0, 3.0])),
        ("CONORAT ", float_arr([2.0, 3.0])),
        ("CONDEPTH", float_arr([2.0, 3.0])),
        ("CONVTUB ", float_arr([2.0, 3.0])),
        ("CONOTUB ", float_arr([2.0, 3.0])),
        ("CONGTUB ", float_arr([2.0, 3.0])),
        ("CONWTUB ", float_arr([2.0, 3.0])),
        ("CONPRES ", float_arr([0.0, 0.0])),
    ]


def make_rft_file(tmp_path, contents):
    file = tmp_path / "CASE.RFT"
    resfo.write(
        file,
        contents,
    )
    return ResdataRFTFile(str(file))


def test_that_reading_an_rft_cell_results_in_the_expected_values(tmp_path):
    rft_file = make_rft_file(
        tmp_path,
        [
            *cell_start(),
            ("PRESSURE", float_arr([100.0, 200.0])),
            ("SWAT    ", float_arr([0.1, 0.2])),
            ("SGAS    ", float_arr([0.3, 0.4])),
            ("DEPTH   ", float_arr([20.0, 30.0])),
            *cell_start(),
            ("PRESSURE", float_arr([101.0, 201.0])),
            ("SWAT    ", float_arr([0.01, 0.01])),
            ("SGAS    ", float_arr([0.01, 0.01])),
            ("DEPTH   ", float_arr([21.0, 31.0])),
        ],
    )

    assert len(rft_file) == 2
    assert rft_file.size() == 2
    assert rft_file.get_num_wells() == 1

    node = rft_file[0]
    assert node.is_RFT()
    assert not node.is_PLT()
    assert not node.is_SEGMENT()
    assert not node.is_MSW()
    assert len(node) == 2
    assert node[0].pressure == 100.0
    assert node[1].pressure == 200.0
    assert node[0].swat == pytest.approx(0.1)
    assert node[1].swat == pytest.approx(0.2)
    assert node[0].sgas == pytest.approx(0.3)
    assert node[1].sgas == pytest.approx(0.4)
    assert node[0].soil == pytest.approx(0.6)
    assert node[1].soil == pytest.approx(0.4)
    assert node[0].get_ijk() == (0, 0, 0)
    assert node[1].get_ijk() == (1, 0, 1)
    assert node[0].depth == 20.0
    assert node[1].depth == 30.0


def test_that_size_counts_matching_cells(tmp_path):
    file = tmp_path / "CASE.RFT"
    resfo.write(
        file,
        [
            *cell_start(well_name=f"WELL1", date=(1, 1, 2000)),
            ("PRESSURE", float_arr([100.0, 200.0])),
            ("SWAT    ", float_arr([0.1, 0.2])),
            ("SGAS    ", float_arr([0.3, 0.4])),
            ("DEPTH   ", float_arr([20.0, 30.0])),
            *cell_start(well_name=f"WELL2", date=(2, 2, 2001)),
            ("PRESSURE", float_arr([101.0, 201.0])),
            ("SWAT    ", float_arr([0.01, 0.01])),
            ("SGAS    ", float_arr([0.01, 0.01])),
            ("DEPTH   ", float_arr([21.0, 31.0])),
            *cell_start(well_name=f"WELL2", date=(2, 3, 2001)),
            ("PRESSURE", float_arr([101.0, 201.0])),
            ("SWAT    ", float_arr([0.01, 0.01])),
            ("SGAS    ", float_arr([0.01, 0.01])),
            ("DEPTH   ", float_arr([21.0, 31.0])),
        ],
    )
    rft_file = ResdataRFTFile(str(file))
    assert rft_file.size() == 3
    assert rft_file.size(date=datetime.date(2000, 1, 1)) == 1
    assert rft_file.size(date=datetime.date(2001, 2, 2)) == 1
    assert rft_file.size(well="WELL1") == 1
    assert rft_file.size(well="WELL2") == 2
    assert rft_file.size(well="WELL*") == 3
    assert rft_file.size(well=" WELL*") == 0
    assert rft_file.get_num_wells() == 2
    assert rft_file.get_headers() == [
        ("WELL1", datetime.date(2000, 1, 1)),
        ("WELL2", datetime.date(2001, 2, 2)),
        ("WELL2", datetime.date(2001, 3, 2)),
    ]


def test_that_reading_a_plt_cell_results_in_the_expected_values(tmp_path):
    rft_file = make_rft_file(
        tmp_path,
        [
            *cell_start(data_category=b"P"),
            ("CONWRAT ", float_arr([2.0, 3.0])),
            ("CONGRAT ", float_arr([4.0, 5.0])),
            ("CONORAT ", float_arr([6.0, 7.0])),
            ("CONDEPTH", float_arr([2000.0, 3000.0])),
            ("CONVTUB ", float_arr([8.0, 9.0])),
            ("CONOTUB ", float_arr([10.0, 11.0])),
            ("CONGTUB ", float_arr([12.0, 13.0])),
            ("CONWTUB ", float_arr([14.0, 15.0])),
            ("CONPRES ", float_arr([20.0, 30.0])),
        ],
    )
    node = rft_file[0]
    assert repr(node) == repr(rft_file.iget(0))
    assert repr(node) == repr(rft_file.get("WELL1", datetime.date(2000, 1, 1)))
    assert node.get_date() == datetime.date(2000, 1, 1)
    assert node.get_well_name() == "WELL1"
    assert not node.is_RFT()
    assert node.is_PLT()
    assert not node.is_SEGMENT()
    assert not node.is_MSW()
    assert repr(node[0]) == repr(node.iget(0))
    assert repr(node[0]) == repr(node.ijkget((0, 0, 0)))
    assert repr(node[1]) == repr(node.ijkget((1, 0, 1)))
    assert node[0].pressure == 20.0
    assert node[1].pressure == 30.0
    assert node[0].wrat == 2.0
    assert node[1].wrat == 3.0
    assert node[0].grat == 4.0
    assert node[1].grat == 5.0
    assert node[0].orat == 6.0
    assert node[1].orat == 7.0
    assert node[0].flowrate == 8.0
    assert node[1].flowrate == 9.0
    assert node[0].oil_flowrate == 10.0
    assert node[1].oil_flowrate == 11.0
    assert node[0].gas_flowrate == 12.0
    assert node[1].gas_flowrate == 13.0
    assert node[0].water_flowrate == 14.0
    assert node[1].water_flowrate == 15.0
    assert node[0].get_ijk() == (0, 0, 0)
    assert node[1].get_ijk() == (1, 0, 1)
    assert node[0].depth == 2000.0
    assert node[1].depth == 3000.0


def test_that_reading_a_plt_cell_with_zero_sum_conpres_uses_pressure(tmp_path):
    rft_file = make_rft_file(
        tmp_path,
        [
            *cell_start(data_category=b"RP      "),
            ("PRESSURE", float_arr([100.0, 200.0])),
            *plt_fields(),
        ],
    )
    node = rft_file[0]
    assert node[0].pressure == 100.0
    assert node[1].pressure == 200.0


def test_that_reading_a_plt_cell_with_zero_sum_conpres_and_no_pressure_uses_zero(
    tmp_path,
):
    rft_file = make_rft_file(
        tmp_path,
        [
            *cell_start(data_category=b"P      "),
            *plt_fields(),
        ],
    )
    node = rft_file[0]
    assert node[0].pressure == 0.0
    assert node[1].pressure == 0.0


def test_that_rft_node_is_msw_is_based_on_conlenst_kw(tmp_path):
    rft_file = make_rft_file(
        tmp_path,
        [
            *cell_start(data_category=b"P"),
            ("CONLENST", float_arr([33.0, 34.0])),
            ("CONLENEN", float_arr([35.0, 36.0])),
            *plt_fields(),
            *cell_start(),
            ("PRESSURE", float_arr([101.0, 201.0])),
            ("SWAT    ", float_arr([199.0, 2198.0])),
            ("SGAS    ", float_arr([140.0, 141.0])),
            ("DEPTH   ", float_arr([21.0, 31.0])),
        ],
    )

    # is_MSW() returns True if CONLENST keyword is in
    # the cell for backwards compatibility
    assert rft_file[0].is_MSW()
    assert not rft_file[1].is_MSW()
    assert rft_file[0][0].conn_start == 33.0
    assert rft_file[0][0].conn_end == 35.0


def test_that_conlen_defaults_to_zero(tmp_path):
    rft_file = make_rft_file(
        tmp_path,
        [
            *cell_start(data_category=b"P"),
            *plt_fields(),
        ],
    )

    assert rft_file[0][0].conn_start == 0.0


def test_that_msw_cells_are_sorted_by_connection_start(tmp_path):
    rft_file = make_rft_file(
        tmp_path,
        [
            *cell_start(data_category=b"P"),
            # first conn is start is after second
            ("CONLENST", float_arr([3.0, 1.0])),
            ("CONLENEN", float_arr([4.0, 2.0])),
            *plt_fields(),
            *cell_start(data_category=b"P"),
            *plt_fields(),
        ],
    )

    # MSW cell gets sorted by conn_start
    assert rft_file[0].is_MSW()
    assert rft_file[0][0].conn_start == 1.0
    assert rft_file[0][0].conn_end == 2.0
    assert rft_file[0][0].wrat == 3.0
    assert rft_file[0][1].conn_start == 3.0
    assert rft_file[0][1].conn_end == 4.0
    assert rft_file[0][1].wrat == 2.0

    # MSW cell order is same as in file
    assert not rft_file[1].is_MSW()
    assert rft_file[1][0].wrat == 2.0
    assert rft_file[1][1].wrat == 3.0
