import pytest
from resdata import FileMode


def test_that_the_file_mode_enum_exposes_the_expected_members():
    assert set(FileMode.__members__) == {"DEFAULT", "CLOSE_STREAM", "WRITABLE"}


@pytest.mark.parametrize(
    "mode, value",
    [
        (FileMode.DEFAULT, 0),
        (FileMode.CLOSE_STREAM, 1),
        (FileMode.WRITABLE, 2),
    ],
)
def test_that_file_mode_members_have_the_expected_integer_values(mode, value):
    assert int(mode) == value


def test_that_or_combines_flags():
    combined = FileMode.CLOSE_STREAM | FileMode.WRITABLE

    assert isinstance(combined, FileMode)
    assert int(combined) == 3


def test_that_or_with_default_is_a_no_op():
    assert FileMode.WRITABLE | FileMode.DEFAULT == FileMode.WRITABLE


def test_that_and_extracts_a_set_flag():
    combined = FileMode.CLOSE_STREAM | FileMode.WRITABLE

    assert combined & FileMode.WRITABLE == FileMode.WRITABLE
    assert combined & FileMode.CLOSE_STREAM == FileMode.CLOSE_STREAM


def test_that_and_of_disjoint_flags_is_default():
    assert FileMode.CLOSE_STREAM & FileMode.WRITABLE == FileMode.DEFAULT


def test_that_xor_toggles_flags():
    assert FileMode.CLOSE_STREAM ^ FileMode.CLOSE_STREAM == FileMode.DEFAULT
    assert int(FileMode.CLOSE_STREAM ^ FileMode.WRITABLE) == 3


def test_that_xor_can_clear_a_single_flag():
    combined = FileMode.CLOSE_STREAM | FileMode.WRITABLE

    assert combined ^ FileMode.WRITABLE == FileMode.CLOSE_STREAM


def test_that_invert_flips_all_bits():
    assert int(~FileMode.DEFAULT) == -1
    assert int(~FileMode.CLOSE_STREAM) == -2


def test_that_inverting_twice_is_the_identity():
    assert ~~FileMode.WRITABLE == FileMode.WRITABLE


def test_that_a_flag_masks_out_of_a_combination_via_invert_and_and():
    combined = FileMode.CLOSE_STREAM | FileMode.WRITABLE

    assert combined & ~FileMode.WRITABLE == FileMode.CLOSE_STREAM
