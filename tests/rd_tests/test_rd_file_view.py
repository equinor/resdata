import datetime

import pytest
from resdata import FileMode, ResDataType
from resdata.resfile import FortIO, ResdataKW, open_rd_file, openFortIO


def _kw(name, dtype, values):
    kw = ResdataKW(name, len(values), dtype)
    for index, value in enumerate(values):
        kw[index] = value
    return kw


def _intehead(day, month, year):
    header = ResdataKW("INTEHEAD", 67, ResDataType.RD_INT)
    header[64] = day
    header[65] = month
    header[66] = year
    return header


@pytest.fixture
def sample_file(tmp_path):
    """A file with two PRESSURE and two SWAT keywords surrounding a header:
    SEQNUM, PRESSURE, SWAT, PRESSURE, SWAT
    """
    path = str(tmp_path / "TEST")
    with openFortIO(path, mode=FortIO.WRITE_MODE) as f:
        _kw("SEQNUM", ResDataType.RD_INT, [10]).fwrite(f)
        _kw("PRESSURE", ResDataType.RD_FLOAT, [1.0, 2.0, 3.0]).fwrite(f)
        _kw("SWAT", ResDataType.RD_FLOAT, [0.1, 0.2, 0.3]).fwrite(f)
        _kw("PRESSURE", ResDataType.RD_FLOAT, [4.0, 5.0, 6.0]).fwrite(f)
        _kw("SWAT", ResDataType.RD_FLOAT, [0.4, 0.5, 0.6]).fwrite(f)
    return path


@pytest.fixture
def restart_file(tmp_path):
    """A restart file with two report steps."""
    path = str(tmp_path / "R.UNRST")
    with openFortIO(path, mode=FortIO.WRITE_MODE) as f:
        _kw("SEQNUM", ResDataType.RD_INT, [10]).fwrite(f)
        _intehead(1, 1, 2000).fwrite(f)
        _kw("PRESSURE", ResDataType.RD_FLOAT, [1.0, 2.0, 3.0]).fwrite(f)
        _kw("SEQNUM", ResDataType.RD_INT, [20]).fwrite(f)
        _intehead(1, 1, 2010).fwrite(f)
        _kw("PRESSURE", ResDataType.RD_FLOAT, [4.0, 5.0, 6.0]).fwrite(f)
    return path


@pytest.fixture
def restart_file_with_days(tmp_path):
    """A restart file with two report steps carrying simulation-day headers.

    ``DOUBHEAD`` index 0 holds the number of simulation days, which is what the
    ``restart_view(sim_days=...)`` lookup matches against.
    """
    path = str(tmp_path / "RD.UNRST")
    with openFortIO(path, mode=FortIO.WRITE_MODE) as f:
        _kw("SEQNUM", ResDataType.RD_INT, [10]).fwrite(f)
        _intehead(1, 1, 2000).fwrite(f)
        _kw("DOUBHEAD", ResDataType.RD_DOUBLE, [10.0]).fwrite(f)
        _kw("PRESSURE", ResDataType.RD_FLOAT, [1.0, 2.0, 3.0]).fwrite(f)
        _kw("SEQNUM", ResDataType.RD_INT, [20]).fwrite(f)
        _intehead(1, 1, 2010).fwrite(f)
        _kw("DOUBHEAD", ResDataType.RD_DOUBLE, [20.0]).fwrite(f)
        _kw("PRESSURE", ResDataType.RD_FLOAT, [4.0, 5.0, 6.0]).fwrite(f)
    return path


def test_that_len_counts_all_keywords(sample_file):
    with open_rd_file(sample_file) as rd_file:
        assert len(rd_file.global_view) == 5


def test_that_the_keywords_are_contained_in_the_view(sample_file):
    with open_rd_file(sample_file) as rd_file:
        view = rd_file.global_view
        assert "PRESSURE" in view
        assert "SWAT" in view
        assert "SEQNUM" in view
        assert "NOSUCHKW" not in view


def test_that_num_keywords_counts_occurrences(sample_file):
    with open_rd_file(sample_file) as rd_file:
        view = rd_file.global_view
        assert view.num_keywords("PRESSURE") == 2
        assert view.num_keywords("SWAT") == 2
        assert view.num_keywords("SEQNUM") == 1
        assert view.num_keywords("NOSUCHKW") == 0


def test_unique_size_and_unique_kw(sample_file):
    with open_rd_file(sample_file) as rd_file:
        view = rd_file.global_view
        assert view.unique_size() == 3
        assert view.unique_kw() == ["SEQNUM", "PRESSURE", "SWAT"]


def test_iget_named_kw_returns_the_arrays(sample_file):
    with open_rd_file(sample_file) as rd_file:
        view = rd_file.global_view
        assert list(view.iget_named_kw("PRESSURE", 0)) == pytest.approx([1.0, 2.0, 3.0])
        assert list(view.iget_named_kw("PRESSURE", 1)) == pytest.approx([4.0, 5.0, 6.0])


def test_iget_named_kw_on_missing_name_raises_keyerror(sample_file):
    with open_rd_file(sample_file) as rd_file:
        with pytest.raises(KeyError):
            rd_file.global_view.iget_named_kw("NOSUCHKW", 0)


def test_iget_named_kw_with_out_of_range_index_raises_indexerror(sample_file):
    with open_rd_file(sample_file) as rd_file:
        with pytest.raises(IndexError):
            rd_file.global_view.iget_named_kw("PRESSURE", 2)


def test_iget_named_kw_with_negative_index_raises_indexerror(sample_file):
    with open_rd_file(sample_file) as rd_file:
        with pytest.raises(IndexError):
            rd_file.global_view.iget_named_kw("PRESSURE", -1)


def test_getitem_by_index(sample_file):
    with open_rd_file(sample_file) as rd_file:
        view = rd_file.global_view
        assert view[0].name == "SEQNUM"
        assert view[1].name == "PRESSURE"
        assert view[-1].name == "SWAT"


def test_getitem_with_out_of_range_index_raises_indexerror(sample_file):
    with open_rd_file(sample_file) as rd_file:
        with pytest.raises(IndexError):
            rd_file.global_view[5]


def test_getitem_by_keyword_returns_all_occurrences(sample_file):
    with open_rd_file(sample_file) as rd_file:
        pressures = rd_file.global_view["PRESSURE"]
        assert len(pressures) == 2
        assert list(pressures[1]) == pytest.approx([4.0, 5.0, 6.0])


def test_getitem_unknown_keyword_raises_keyerror(sample_file):
    with open_rd_file(sample_file) as rd_file:
        with pytest.raises(KeyError):
            rd_file.global_view["NOSUCHKW"]


def test_getitem_wrong_type_raises_typeerror(sample_file):
    with open_rd_file(sample_file) as rd_file:
        with pytest.raises(TypeError):
            rd_file.global_view[1.5]


def test_getitem_slice(sample_file):
    with open_rd_file(sample_file) as rd_file:
        names = [kw.name for kw in rd_file.global_view[0:3]]
        assert names == ["SEQNUM", "PRESSURE", "SWAT"]


def test_getitem_slice_with_nonzero_start(sample_file):
    with open_rd_file(sample_file) as rd_file:
        names = [kw.name for kw in rd_file.global_view[1:4]]
        assert names == ["PRESSURE", "SWAT", "PRESSURE"]


def test_getitem_slice_with_step(sample_file):
    with open_rd_file(sample_file) as rd_file:
        names = [kw.name for kw in rd_file.global_view[::2]]
        assert names == ["SEQNUM", "SWAT", "SWAT"]


def test_getitem_slice_with_start_and_step(sample_file):
    with open_rd_file(sample_file) as rd_file:
        names = [kw.name for kw in rd_file.global_view[1::2]]
        assert names == ["PRESSURE", "PRESSURE"]


def test_getitem_slice_reverse(sample_file):
    with open_rd_file(sample_file) as rd_file:
        names = [kw.name for kw in rd_file.global_view[2::-1]]
        assert names == ["SWAT", "PRESSURE", "SEQNUM"]


def test_block_view(sample_file):
    with open_rd_file(sample_file) as rd_file:
        block = rd_file.global_view.block_view("PRESSURE", 1)
        assert "PRESSURE" in block


def test_that_block_view_with_unknown_keyword_raises_keyerror(sample_file):
    with open_rd_file(sample_file) as rd_file:
        with pytest.raises(KeyError):
            rd_file.global_view.block_view("NOSUCHKW", 0)


def test_that_block_view_with_out_of_range_index_raises_indexerror(sample_file):
    with open_rd_file(sample_file) as rd_file:
        with pytest.raises(IndexError):
            rd_file.global_view.block_view("PRESSURE", 5)


def test_block_view_with_negative_index_counts_from_end(sample_file):
    with open_rd_file(sample_file) as rd_file:
        block = rd_file.global_view.block_view("PRESSURE", -1)
        assert list(block[0]) == pytest.approx([4.0, 5.0, 6.0])


def test_block_view_with_out_of_range_negative_index_raises_indexerror(sample_file):
    with open_rd_file(sample_file) as rd_file:
        with pytest.raises(IndexError):
            rd_file.global_view.block_view("PRESSURE", -3)


def test_block_view2(sample_file):
    with open_rd_file(sample_file) as rd_file:
        block = rd_file.global_view.block_view2("SEQNUM", "SWAT", 0)
        assert "PRESSURE" in block


def test_block_view2_with_unknown_keyword_raises_keyerror(sample_file):
    with open_rd_file(sample_file) as rd_file:
        with pytest.raises(KeyError):
            rd_file.global_view.block_view2("NOSUCHKW", "SWAT", 0)


def test_block_view2_with_negative_index_counts_from_end(sample_file):
    with open_rd_file(sample_file) as rd_file:
        block = rd_file.global_view.block_view2("SEQNUM", "SWAT", -1)
        assert "PRESSURE" in block


def _kw_names(view):
    return [view[i].name for i in range(len(view))]


def test_block_view_returns_block_up_to_next_occurrence(sample_file):
    with open_rd_file(sample_file) as rd_file:
        block = rd_file.global_view.block_view("PRESSURE", 0)
        assert _kw_names(block) == ["PRESSURE", "SWAT"]


def test_block_view_last_block_extends_to_end_of_file(sample_file):
    with open_rd_file(sample_file) as rd_file:
        block = rd_file.global_view.block_view("PRESSURE", 1)
        assert _kw_names(block) == ["PRESSURE", "SWAT"]


def test_block_view2_with_none_start_kw_reads_from_start_of_file(sample_file):
    with open_rd_file(sample_file) as rd_file:
        block = rd_file.global_view.block_view2(None, "PRESSURE", 0)
        assert _kw_names(block) == ["SEQNUM"]


def test_block_view2_with_none_stop_kw_reads_to_end_of_file(sample_file):
    with open_rd_file(sample_file) as rd_file:
        block = rd_file.global_view.block_view2("SEQNUM", None, 0)
        assert _kw_names(block) == ["SEQNUM", "PRESSURE", "SWAT", "PRESSURE", "SWAT"]


def test_block_view2_with_none_start_and_stop_returns_all_keywords(sample_file):
    with open_rd_file(sample_file) as rd_file:
        block = rd_file.global_view.block_view2(None, None, 0)
        assert _kw_names(block) == ["SEQNUM", "PRESSURE", "SWAT", "PRESSURE", "SWAT"]


def test_that_restart_view_iget_named_kw_with_index_returns_the_array(restart_file):
    with open_rd_file(restart_file) as rd_file:
        view = rd_file.global_view
        assert list(view.restart_view(seqnum_index=0).iget_named_kw("PRESSURE", 0)) == (
            pytest.approx([1.0, 2.0, 3.0])
        )
        assert list(view.restart_view(seqnum_index=1).iget_named_kw("PRESSURE", 0)) == (
            pytest.approx([4.0, 5.0, 6.0])
        )


def test_restart_view_by_report_step(restart_file):
    with open_rd_file(restart_file) as rd_file:
        view = rd_file.global_view.restart_view(report_step=20)
        assert list(view.iget_named_kw("PRESSURE", 0)) == pytest.approx([4.0, 5.0, 6.0])


def test_restart_view_by_sim_time(restart_file):
    with open_rd_file(restart_file) as rd_file:
        view = rd_file.global_view.restart_view(sim_time=datetime.date(2000, 1, 1))
        assert list(view.iget_named_kw("PRESSURE", 0)) == pytest.approx([1.0, 2.0, 3.0])


def test_restart_view_without_criteria_raises_valueerror(restart_file):
    with open_rd_file(restart_file) as rd_file:
        with pytest.raises(ValueError):
            rd_file.global_view.restart_view()


def test_restart_view_by_missing_report_step_raises(restart_file):
    with open_rd_file(restart_file) as rd_file:
        with pytest.raises(
            ValueError, match="No such restart block could be identified"
        ):
            rd_file.global_view.restart_view(report_step=999)


def test_restart_view_by_missing_sim_time_raises(restart_file):
    with open_rd_file(restart_file) as rd_file:
        with pytest.raises(
            ValueError, match="No such restart block could be identified"
        ):
            rd_file.global_view.restart_view(sim_time=datetime.date(1999, 1, 1))


def test_restart_view_by_sim_days_returns_the_matching_block(restart_file_with_days):
    with open_rd_file(restart_file_with_days) as rd_file:
        view = rd_file.global_view

        first = view.restart_view(sim_days=10.0)
        assert list(first.iget_named_kw("PRESSURE", 0)) == pytest.approx(
            [1.0, 2.0, 3.0]
        )

        second = view.restart_view(sim_days=20.0)
        assert list(second.iget_named_kw("PRESSURE", 0)) == pytest.approx(
            [4.0, 5.0, 6.0]
        )


def test_restart_view_by_missing_sim_days_raises(restart_file_with_days):
    with open_rd_file(restart_file_with_days) as rd_file:
        with pytest.raises(
            ValueError, match="No such restart block could be identified"
        ):
            # 5.0 is smaller than the first block's day count
            rd_file.global_view.restart_view(sim_days=5.0)


def test_file_mode_values():
    assert int(FileMode.DEFAULT) == 0
    assert int(FileMode.CLOSE_STREAM) == 1
    assert int(FileMode.WRITABLE) == 2


def test_file_mode_bitwise_or_combines_flags():
    combined = FileMode.WRITABLE | FileMode.CLOSE_STREAM
    assert int(combined) == 3


@pytest.mark.parametrize(
    "flags",
    [
        FileMode.DEFAULT,
        FileMode.CLOSE_STREAM,
        FileMode.WRITABLE,
        FileMode.WRITABLE | FileMode.CLOSE_STREAM,
    ],
)
def test_view_reads_identically_for_all_file_modes(sample_file, flags):
    with open_rd_file(sample_file, flags=flags) as rd_file:
        view = rd_file.global_view
        assert len(view) == 5
        assert view.num_keywords("PRESSURE") == 2
        assert list(view.iget_named_kw("PRESSURE", 1)) == pytest.approx([4.0, 5.0, 6.0])


def test_close_stream_allows_repeated_view_reads(sample_file):
    """With CLOSE_STREAM the file handle is closed between accesses; the
    view must transparently reopen it."""
    with open_rd_file(sample_file, flags=FileMode.CLOSE_STREAM) as rd_file:
        view = rd_file.global_view
        first = list(view.iget_named_kw("PRESSURE", 0))
        _ = list(view.iget_named_kw("SWAT", 1))
        second = list(view.iget_named_kw("PRESSURE", 0))
        assert first == pytest.approx([1.0, 2.0, 3.0])
        assert second == pytest.approx(first)
