import datetime
import gc

import pytest
from resdata.resfile import ResdataFile, ResdataFileView, ResdataKW

SIMPLE_UNRST = "test-data/local/ECLIPSE/simple/SIMPLE.UNRST"
UNIQUE_KEYWORDS = [
    "SEQNUM",
    "INTEHEAD",
    "LOGIHEAD",
    "DOUBHEAD",
    "IGRP",
    "SGRP",
    "XGRP",
    "ZGRP",
    "STARTSOL",
    "1OVERBO",
    "1OVERBW",
    "OILKR",
    "OIL_DEN",
    "OIL_VISC",
    "PRESSURE",
    "SWAT",
    "WATKR",
    "WAT_DEN",
    "WAT_VISC",
    "ENDSOL",
]


@pytest.fixture
def simple_file():
    rd_file = ResdataFile(SIMPLE_UNRST)
    try:
        yield rd_file
    finally:
        rd_file.close()


@pytest.fixture
def simple_view(simple_file):
    return simple_file.global_view


def test_len_and_repr_of_global_view_are_exact(simple_view):
    assert isinstance(simple_view, ResdataFileView)
    assert len(simple_view) == 80
    assert repr(simple_view).startswith("ResdataFileView(size=80)")


def test_unique_keywords_are_ordered_and_exact(simple_view):
    assert simple_view.unique_size() == 20
    assert simple_view.uniqueSize() == 20
    assert simple_view.unique_kw() == UNIQUE_KEYWORDS


def test_contains_accepts_text_and_bytes_keywords(simple_view):
    assert "SEQNUM" in simple_view
    assert b"SEQNUM" in simple_view
    assert "NO_SUCH" not in simple_view
    assert b"NO_SUCH" not in simple_view


def test_num_keywords_counts_each_occurrence(simple_view):
    assert simple_view.num_keywords("SEQNUM") == 4
    assert simple_view.numKeywords("SEQNUM") == 4
    assert simple_view.num_keywords(b"SWAT") == 4
    assert simple_view.num_keywords("NO_SUCH") == 0


def test_getitem_by_integer_negative_index_and_slice(simple_view):
    assert simple_view[0].name == "SEQNUM"
    assert simple_view[-1].name == "ENDSOL"
    assert [kw.name for kw in simple_view[18:22]] == [
        "WAT_VISC",
        "ENDSOL",
        "SEQNUM",
        "INTEHEAD",
    ]


def test_getitem_by_keyword_string_and_bytes_returns_kw_lists(simple_view):
    swat_keywords = simple_view["SWAT"]
    seqnum_keywords = simple_view[b"SEQNUM"]
    assert len(swat_keywords) == 4
    assert all(isinstance(kw, ResdataKW) and kw.name == "SWAT" for kw in swat_keywords)
    assert [kw[0] for kw in seqnum_keywords] == [1, 2, 3, 4]


def test_missing_keyword_and_bad_indices_raise_public_exceptions(simple_view):
    with pytest.raises(IndexError):
        simple_view[80]
    with pytest.raises(IndexError):
        simple_view[-81]
    with pytest.raises(KeyError):
        simple_view["NO_SUCH"]
    with pytest.raises(KeyError):
        simple_view.iget_named_kw("NO_SUCH", 0)
    with pytest.raises(IndexError):
        simple_view.iget_named_kw("SEQNUM", 4)


def test_iget_named_kw_handles_bytes_and_negative_index(simple_view):
    first = simple_view.iget_named_kw(b"SEQNUM", 0)
    last = simple_view.iget_named_kw("SEQNUM", -1)
    assert first.name == "SEQNUM"
    assert len(first) == 1
    assert first[0] == 1
    assert last.name == "SEQNUM"
    assert last[0] == 1


def test_block_views_preserve_boundaries_and_none_limits(simple_view):
    second = simple_view.block_view("SEQNUM", 1)
    last = simple_view.blockView("SEQNUM", -1)
    prefix = simple_view.block_view2(None, "ENDSOL", 0)
    suffix = simple_view.blockView2("SEQNUM", None, -1)
    assert len(second) == 20
    assert second["SEQNUM"][0][0] == 2
    assert len(last) == 20
    assert last["SEQNUM"][0][0] == 1
    assert len(prefix) == 19
    assert "ENDSOL" not in prefix
    assert prefix["SEQNUM"][0][0] == 1
    assert len(suffix) == 20
    assert suffix["SEQNUM"][0][0] == 4


def test_restart_view_selectors_and_returned_references_keep_parent_alive():
    def make_references():
        rd_file = ResdataFile(SIMPLE_UNRST)
        return (
            rd_file.global_view.restart_view(report_step=3),
            rd_file.global_view.restartView(sim_time=datetime.datetime(2017, 3, 2)),
            rd_file.global_view.restart_view(sim_days=30.0),
            rd_file.global_view["SEQNUM"][0],
            rd_file.global_view.block_view("SEQNUM", 2),
        )

    by_step, by_date, by_days, keyword, block = make_references()
    gc.collect()
    assert by_step["SEQNUM"][0][0] == 3
    assert by_date["SEQNUM"][0][0] == 4
    assert by_days["SEQNUM"][0][0] == 2
    assert keyword[0] == 1
    assert block["SEQNUM"][0][0] == 3
