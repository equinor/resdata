import pytest
from resdata import ResDataType
from resdata.resfile import ResdataKW


def write_grdecl_file(tmp_path, content, name="test.grdecl"):
    path = tmp_path / name
    path.write_text(content)
    return path


def test_that_fseek_finds_keyword_at_start_of_file(tmp_path):
    path = write_grdecl_file(tmp_path, "PERMX\n1 2 3 /\n")
    with open(path) as f:
        assert ResdataKW.fseek_grdecl(f, "PERMX")
        assert f.read(5) == "PERMX"


def test_that_fseek_returns_false_for_missing_keyword(tmp_path):
    path = write_grdecl_file(tmp_path, "PERMX\n1 2 3 /\n")
    with open(path) as f:
        assert not ResdataKW.fseek_grdecl(f, "PERMY")


def test_that_fseek_leaves_position_unchanged_on_failure_without_rewind(tmp_path):
    path = write_grdecl_file(tmp_path, "PERMX\n1 2 3 /\nPERMZ\n4 5 6 /\n")
    with open(path) as f:
        f.readline()  # Position ourselves part-way through the file.
        pos = f.tell()
        assert not ResdataKW.fseek_grdecl(f, "PERMY")
        assert f.tell() == pos


def test_that_fseek_positions_file_at_start_of_matched_keyword(tmp_path):
    path = write_grdecl_file(tmp_path, "  PERMX\n1 2 3 /\n")
    with open(path) as f:
        assert ResdataKW.fseek_grdecl(f, "PERMX")
        assert f.read() == "PERMX\n1 2 3 /\n"


def test_that_fseek_allows_leading_whitespace_before_keyword(tmp_path):
    path = write_grdecl_file(tmp_path, "   PERMX\n1 2 3 /\n")
    with open(path) as f:
        assert ResdataKW.fseek_grdecl(f, "PERMX")


def test_that_fseek_skips_blank_lines(tmp_path):
    path = write_grdecl_file(tmp_path, "\n\n\nPERMX\n1 2 3 /\n")
    with open(path) as f:
        assert ResdataKW.fseek_grdecl(f, "PERMX")


def test_that_fseek_skips_commented_out_keyword(tmp_path):
    path = write_grdecl_file(tmp_path, "--PERMX\n1 2 3 /\n")
    with open(path) as f:
        assert not ResdataKW.fseek_grdecl(f, "PERMX")


def test_that_fseek_skips_comment_line_with_leading_whitespace(tmp_path):
    path = write_grdecl_file(tmp_path, "  -- PERMX\nPERMX\n1 2 3 /\n")
    with open(path) as f:
        assert ResdataKW.fseek_grdecl(f, "PERMX")
        assert f.read(5) == "PERMX"


def test_that_fseek_does_not_find_keyword_appearing_after_a_comment_marker_on_same_line(
    tmp_path,
):
    # A comment marker anywhere as the *first* token comments out the whole
    # line, even if a real keyword-looking token follows on the same line.
    path = write_grdecl_file(tmp_path, "--PERMY  PERMZ\nPERMZ\n1 2 3 /\n")
    with open(path) as f:
        assert not ResdataKW.fseek_grdecl(f, "PERMY")
    with open(path) as f:
        assert ResdataKW.fseek_grdecl(f, "PERMZ")
        # The match is the PERMZ on the second, uncommented line.
        assert f.read() == "PERMZ\n1 2 3 /\n"


def test_that_fseek_only_considers_first_token_on_a_line(tmp_path):
    # PERMXYZ is not the first token on its line and must not be found.
    path = write_grdecl_file(tmp_path, "MARKER   PERMXYZ\n")
    with open(path) as f:
        assert ResdataKW.fseek_grdecl(f, "MARKER")
    with open(path) as f:
        assert not ResdataKW.fseek_grdecl(f, "PERMXYZ")


def test_that_fseek_requires_exact_token_match(tmp_path):
    # Neither a prefix nor a suffix match should be accepted.
    path = write_grdecl_file(tmp_path, "PERMXYZ\n1 2 3 /\n")
    with open(path) as f:
        assert not ResdataKW.fseek_grdecl(f, "PERMX")

    path2 = write_grdecl_file(tmp_path, "PERMX\n1 2 3 /\n", name="test2.grdecl")
    with open(path2) as f:
        assert not ResdataKW.fseek_grdecl(f, "PERMXYZ")


def test_that_fseek_is_case_sensitive(tmp_path):
    path = write_grdecl_file(tmp_path, "permx\n1 2 3 /\n")
    with open(path) as f:
        assert not ResdataKW.fseek_grdecl(f, "PERMX")


def test_that_fseek_does_not_find_keyword_before_current_position(tmp_path):
    path = write_grdecl_file(tmp_path, "PERMX\n1 2 3 /\nPERMZ\n4 5 6 /\n")
    with open(path) as f:
        assert ResdataKW.fseek_grdecl(f, "PERMZ")
        assert not ResdataKW.fseek_grdecl(f, "PERMX")


def test_that_fseek_finds_keyword_when_position_is_already_at_line_start(tmp_path):
    # If the current position is already at the (whitespace-only-prefixed)
    # start of a line, that line is still a valid candidate.
    path = write_grdecl_file(tmp_path, "PERMX\n1 2 3 /\n")
    with open(path) as f:
        f.read(0)
        assert ResdataKW.fseek_grdecl(f, "PERMX")


def test_that_fseek_rewind_finds_keyword_before_current_position(tmp_path):
    path = write_grdecl_file(tmp_path, "PERMX\n1 2 3 /\nPERMZ\n4 5 6 /\n")
    with open(path) as f:
        assert ResdataKW.fseek_grdecl(f, "PERMZ")
        assert not ResdataKW.fseek_grdecl(f, "PERMX", rewind=False)
        assert ResdataKW.fseek_grdecl(f, "PERMX", rewind=True)


def test_that_fseek_rewind_leaves_position_unchanged_if_still_not_found(tmp_path):
    path = write_grdecl_file(tmp_path, "PERMX\n1 2 3 /\nPERMZ\n4 5 6 /\n")
    with open(path) as f:
        f.readline()
        pos = f.tell()
        assert not ResdataKW.fseek_grdecl(f, "MISSING", rewind=True)
        assert f.tell() == pos


def test_that_fseek_handles_multiple_consecutive_comments_and_blanks(tmp_path):
    path = write_grdecl_file(
        tmp_path,
        "PERMX\n"
        "\n"
        "--PERMY  PERMZ\n"
        "\n"
        "MARKER   PERMXYZ\n"
        "\n"
        "-- PERMX\n"
        "-- PERMY\n"
        "\n"
        "\n"
        "\n"
        "-- PORO\n"
        "LASTKW\n",
    )
    with open(path) as f:
        assert ResdataKW.fseek_grdecl(f, "PERMX")
        assert not ResdataKW.fseek_grdecl(f, "PERMY")
        assert not ResdataKW.fseek_grdecl(f, "PERMZ")
        assert ResdataKW.fseek_grdecl(f, "MARKER")
        assert not ResdataKW.fseek_grdecl(f, "PERMXYZ")
        assert not ResdataKW.fseek_grdecl(f, "PERMX")
        assert ResdataKW.fseek_grdecl(f, "PERMX", rewind=True)
        assert ResdataKW.fseek_grdecl(f, "LASTKW")


def test_that_fseek_handles_dos_line_endings(tmp_path):
    path = tmp_path / "test.grdecl_dos"
    path.write_bytes(b"PERMX\r\n1 2 3 /\r\nPERMZ\r\n4 5 6 /\r\n")
    with open(path, newline="") as f:
        assert ResdataKW.fseek_grdecl(f, "PERMX")
        assert not ResdataKW.fseek_grdecl(f, "PERMY")
        assert ResdataKW.fseek_grdecl(f, "PERMZ")


def test_that_fseek_returns_false_on_empty_file(tmp_path):
    path = write_grdecl_file(tmp_path, "")
    with open(path) as f:
        assert not ResdataKW.fseek_grdecl(f, "PERMX")


def test_that_fseek_returns_false_at_eof(tmp_path):
    path = write_grdecl_file(tmp_path, "PERMX\n1 2 3 /\n")
    with open(path) as f:
        f.read()
        assert not ResdataKW.fseek_grdecl(f, "PERMX")
        assert ResdataKW.fseek_grdecl(f, "PERMX", rewind=True)


def test_that_fseek_repeated_calls_progress_through_the_file(tmp_path):
    path = write_grdecl_file(
        tmp_path, "PERMX\n1 2 3 /\nPERMY\n4 5 6 /\nPERMZ\n7 8 9 /\n"
    )
    with open(path) as f:
        for name in ("PERMX", "PERMY", "PERMZ"):
            assert ResdataKW.fseek_grdecl(f, name)
            assert f.read(len(name)) == name
        assert not ResdataKW.fseek_grdecl(f, "PERMX")
        assert ResdataKW.fseek_grdecl(f, "PERMX", rewind=True)


def test_that_fseek_allows_leading_spaces_before_keyword(tmp_path):
    path = write_grdecl_file(tmp_path, "          PERMX\n1 2 3 /\n")
    with open(path) as f:
        assert ResdataKW.fseek_grdecl(f, "PERMX")


def test_that_fseek_allows_leading_tabs_before_keyword(tmp_path):
    path = write_grdecl_file(tmp_path, "\t\tPERMX\n1 2 3 /\n")
    with open(path) as f:
        assert ResdataKW.fseek_grdecl(f, "PERMX")


def test_that_fseek_allows_mixed_leading_tabs_and_spaces_before_keyword(tmp_path):
    path = write_grdecl_file(tmp_path, " \t \tPERMX\n1 2 3 /\n")
    with open(path) as f:
        assert ResdataKW.fseek_grdecl(f, "PERMX")


def test_that_fseek_match_position_excludes_leading_whitespace(tmp_path):
    path = write_grdecl_file(tmp_path, "   PERMX\n1 2 3 /\n")
    with open(path) as f:
        assert ResdataKW.fseek_grdecl(f, "PERMX")
        # The three leading spaces are consumed; only "PERMX\n..." remains.
        assert f.read() == "PERMX\n1 2 3 /\n"


def test_that_fseek_finds_keyword_when_starting_position_is_within_leading_whitespace(
    tmp_path,
):
    # If the current position falls inside a run of leading whitespace
    # (i.e. only whitespace precedes it back to the start of the line),
    # that line is still a valid candidate and the keyword ahead is found.
    path = write_grdecl_file(tmp_path, "     PERMX\n1 2 3 /\n")
    with open(path) as f:
        f.read(2)  # Skip past the first two of five leading spaces.
        assert ResdataKW.fseek_grdecl(f, "PERMX")


def test_that_fseek_does_not_find_keyword_on_a_line_with_leading_content(tmp_path):
    # Leading whitespace is fine, but leading *non*-whitespace content
    # before the keyword on the same line disqualifies it as a match.
    path = write_grdecl_file(tmp_path, "MARKER PERMX\n1 2 3 /\n")
    with open(path) as f:
        assert not ResdataKW.fseek_grdecl(f, "PERMX")


def test_that_fseek_allows_leading_whitespace_on_a_comment_line(tmp_path):
    path = write_grdecl_file(tmp_path, "   -- PERMX\nPERMX\n1 2 3 /\n")
    with open(path) as f:
        assert ResdataKW.fseek_grdecl(f, "PERMX")
        # Matches the uncommented PERMX on the second line, not the
        # indented commented-out one on the first.
        assert f.read() == "PERMX\n1 2 3 /\n"


def test_that_fseek_allows_leading_whitespace_after_blank_lines(tmp_path):
    path = write_grdecl_file(tmp_path, "\n\n   \n\t\n   PERMX\n1 2 3 /\n")
    with open(path) as f:
        assert ResdataKW.fseek_grdecl(f, "PERMX")


def test_that_read_grdecl_defaults_to_float_for_unknown_keyword(tmp_path):
    path = write_grdecl_file(tmp_path, "PORO\n1 2 3 /\n")
    with open(path) as f:
        kw = ResdataKW.read_grdecl(f, "PORO")
    assert kw.data_type.is_float()


def test_that_read_grdecl_uses_int_type_for_well_known_int_keywords(tmp_path):
    path = write_grdecl_file(tmp_path, "PVTNUM\n1 2 3 /\n")
    with open(path) as f:
        kw = ResdataKW.read_grdecl(f, "PVTNUM")
    assert kw.data_type.is_int()


@pytest.mark.parametrize(
    "kw", ["PVTNUM", "FIPNUM", "EQLNUM", "FLUXNUM", "MULTNUM", "ACTNUM", "SPECGRID"]
)
def test_that_read_grdecl_resolves_all_builtin_int_keywords(tmp_path, kw):
    path = write_grdecl_file(tmp_path, f"{kw}\n1 2 3 /\n")
    with open(path) as f:
        result = ResdataKW.read_grdecl(f, kw)
    assert result.data_type.is_int()


def test_that_read_grdecl_explicit_type_overrides_int_kw_set_membership(tmp_path):
    # PVTNUM is in int_kw_set, but an explicit rd_type takes precedence.
    path = write_grdecl_file(tmp_path, "PVTNUM\n1.5 2.5 3.5 /\n")
    with open(path) as f:
        kw = ResdataKW.read_grdecl(f, "PVTNUM", rd_type=ResDataType.RD_FLOAT)
    assert kw.data_type.is_float()


def test_that_read_grdecl_explicit_type_overrides_float_default(tmp_path):
    path = write_grdecl_file(tmp_path, "PORO\n1 2 3 /\n")
    with open(path) as f:
        kw = ResdataKW.read_grdecl(f, "PORO", rd_type=ResDataType.RD_INT)
    assert kw.data_type.is_int()


def test_that_read_grdecl_rejects_non_resdatatype_rd_type(tmp_path):
    path = write_grdecl_file(tmp_path, "PORO\n1 2 3 /\n")
    with open(path) as f:
        with pytest.raises(TypeError, match="Expected ResDataType"):
            ResdataKW.read_grdecl(f, "PORO", rd_type="float")


def test_that_read_grdecl_rejects_unsupported_resdatatype(tmp_path):
    path = write_grdecl_file(tmp_path, "PORO\n1 2 3 /\n")
    with open(path) as f:
        with pytest.raises(ValueError, match="invalid"):
            ResdataKW.read_grdecl(f, "PORO", rd_type=ResDataType.RD_DOUBLE)


def test_that_add_int_kw_makes_a_keyword_resolve_to_int(tmp_path):
    path = write_grdecl_file(tmp_path, "MYFLAG\n1 2 3 /\n")
    ResdataKW.add_int_kw("MYFLAG")
    try:
        with open(path) as f:
            kw = ResdataKW.read_grdecl(f, "MYFLAG")
        assert kw.data_type.is_int()
    finally:
        ResdataKW.del_int_kw("MYFLAG")


def test_that_del_int_kw_reverts_a_keyword_to_the_float_default(tmp_path):
    path = write_grdecl_file(tmp_path, "PVTNUM\n1 2 3 /\n")
    ResdataKW.del_int_kw("PVTNUM")
    try:
        with open(path) as f:
            kw = ResdataKW.read_grdecl(f, "PVTNUM")
        assert kw.data_type.is_float()
    finally:
        ResdataKW.add_int_kw("PVTNUM")


def test_that_read_grdecl_resolves_mixed_types_in_the_same_file(tmp_path):
    # A single file with both a well-known int keyword and a float keyword;
    # each read_grdecl() call resolves its own type independently, and the
    # file position correctly advances across both regardless of the
    # differing element widths/formats.
    path = write_grdecl_file(
        tmp_path,
        "PORO\n0.1 0.2 0.3 /\nPVTNUM\n1 2 3 /\nPERMX\n10.0 20.0 30.0 /\n",
    )
    with open(path) as f:
        poro = ResdataKW.read_grdecl(f, "PORO")
        pvtnum = ResdataKW.read_grdecl(f, "PVTNUM")
        permx = ResdataKW.read_grdecl(f, "PERMX")

    assert poro.data_type.is_float()
    assert list(poro.numpy_view()) == pytest.approx([0.1, 0.2, 0.3])

    assert pvtnum.data_type.is_int()
    assert list(pvtnum.numpy_view()) == [1, 2, 3]

    assert permx.data_type.is_float()
    assert list(permx.numpy_view()) == pytest.approx([10.0, 20.0, 30.0])


def test_that_read_grdecl_mixed_types_are_independent_of_search_order(tmp_path):
    # Reading the int keyword before the float keyword works the same way,
    # confirming type resolution does not depend on read order.
    path = write_grdecl_file(tmp_path, "ACTNUM\n1 1 0 /\nPORO\n0.25 0.30 0.35 /\n")
    with open(path) as f:
        actnum = ResdataKW.read_grdecl(f, "ACTNUM")
        poro = ResdataKW.read_grdecl(f, "PORO")

    assert actnum.data_type.is_int()
    assert list(actnum.numpy_view()) == [1, 1, 0]
    assert poro.data_type.is_float()
    assert list(poro.numpy_view()) == pytest.approx([0.25, 0.30, 0.35])


def test_that_read_grdecl_parses_plain_int_values(tmp_path):
    path = write_grdecl_file(tmp_path, "INTKW\n1 2 3 /\n")
    with open(path) as f:
        kw = ResdataKW.read_grdecl(f, "INTKW", rd_type=ResDataType.RD_INT)
    assert list(kw.numpy_view()) == [1, 2, 3]


def test_that_read_grdecl_parses_plain_float_values(tmp_path):
    path = write_grdecl_file(tmp_path, "PORO\n0.1 0.2 0.3 /\n")
    with open(path) as f:
        kw = ResdataKW.read_grdecl(f, "PORO", rd_type=ResDataType.RD_FLOAT)
    assert list(kw.numpy_view()) == pytest.approx([0.1, 0.2, 0.3])


def test_that_read_grdecl_expands_int_multiplier_syntax(tmp_path):
    path = write_grdecl_file(tmp_path, "ACTNUM\n3*1 2*0 /\n")
    with open(path) as f:
        kw = ResdataKW.read_grdecl(f, "ACTNUM", rd_type=ResDataType.RD_INT)
    assert list(kw.numpy_view()) == [1, 1, 1, 0, 0]


def test_that_read_grdecl_expands_float_multiplier_syntax(tmp_path):
    path = write_grdecl_file(tmp_path, "PORO\n4*0.15 /\n")
    with open(path) as f:
        kw = ResdataKW.read_grdecl(f, "PORO", rd_type=ResDataType.RD_FLOAT)
    assert list(kw.numpy_view()) == pytest.approx([0.15] * 4)


def test_that_read_grdecl_multiplier_with_whitespace_around_star_is_malformed(
    tmp_path,
):
    # "N * VALUE" with spaces is two/three separate tokens, none of which is
    # a valid multiplier or a valid plain value on its own.
    path = write_grdecl_file(tmp_path, "PORO\n3 * 0.1 /\n")
    with open(path) as f:
        with pytest.raises(ValueError, match=r'Malformed content:"\*"'):
            ResdataKW.read_grdecl(f, "PORO", rd_type=ResDataType.RD_FLOAT)


def test_that_read_grdecl_ignores_comment_lines_in_data_section(tmp_path):
    path = write_grdecl_file(tmp_path, "PORO\n0.1 0.2\n-- a comment here\n0.3 /\n")
    with open(path) as f:
        kw = ResdataKW.read_grdecl(f, "PORO", rd_type=ResDataType.RD_FLOAT)
    assert list(kw.numpy_view()) == pytest.approx([0.1, 0.2, 0.3])


def test_that_read_grdecl_comment_token_must_match_exactly(tmp_path):
    path = write_grdecl_file(tmp_path, "PORO\n0.1 --FOO 0.3 /\n")
    with open(path) as f:
        with pytest.raises(ValueError, match=r'Malformed content:"--FOO"'):
            ResdataKW.read_grdecl(f, "PORO", rd_type=ResDataType.RD_FLOAT)


def test_that_read_grdecl_strict_raises_on_malformed_int_token(tmp_path):
    path = write_grdecl_file(tmp_path, "INTKW\n1 2 FOO 3 /\n")
    with open(path) as f:
        with pytest.raises(
            ValueError, match=r'Malformed content:"FOO" when reading keyword:INTKW'
        ):
            ResdataKW.read_grdecl(f, "INTKW", rd_type=ResDataType.RD_INT)


def test_that_read_grdecl_strict_raises_on_malformed_float_token(tmp_path):
    path = write_grdecl_file(tmp_path, "FLTKW\n1.0 2.0 BAR 3.0 /\n")
    with open(path) as f:
        with pytest.raises(
            ValueError, match=r'Malformed content:"BAR" when reading keyword:FLTKW'
        ):
            ResdataKW.read_grdecl(f, "FLTKW", rd_type=ResDataType.RD_FLOAT)


def test_that_read_grdecl_non_strict_skips_malformed_tokens(tmp_path):
    path = write_grdecl_file(tmp_path, "SPECGRID\n10 10 3 F /\n")
    with open(path) as f:
        kw = ResdataKW.read_grdecl(
            f, "SPECGRID", strict=False, rd_type=ResDataType.RD_INT
        )
    assert list(kw.numpy_view()) == [10, 10, 3]


def test_that_read_grdecl_raises_for_missing_keyword(tmp_path):
    path = write_grdecl_file(tmp_path, "PORO\n1 2 3 /\n")
    with open(path) as f:
        with pytest.raises(ValueError):
            ResdataKW.read_grdecl(f, "NOTPORO", rd_type=ResDataType.RD_FLOAT)


def test_that_read_grdecl_returns_zero_length_kw_for_empty_body(tmp_path):
    path = write_grdecl_file(tmp_path, "PORO\n/\n")
    with open(path) as f:
        kw = ResdataKW.read_grdecl(f, "PORO", rd_type=ResDataType.RD_FLOAT)
    assert kw.name == "PORO"
    assert len(kw) == 0


def test_that_read_grdecl_with_kw_none_loads_the_next_token_as_header(tmp_path):
    path = write_grdecl_file(tmp_path, "PORO\n0.1 0.2 0.3 /\n")
    with open(path) as f:
        kw = ResdataKW.read_grdecl(f, None, rd_type=ResDataType.RD_FLOAT)
    assert kw.name == "PORO"
    assert list(kw.numpy_view()) == pytest.approx([0.1, 0.2, 0.3])


def test_that_read_grdecl_finds_keyword_anywhere_in_the_file_regardless_of_position(
    tmp_path,
):
    path = write_grdecl_file(tmp_path, "PORO\n0.1 0.2 0.3 /\nPERMX\n10 20 30 /\n")
    with open(path) as f:
        f.seek(0)
        f.readline()  # Skip past PORO's header line entirely.
        f.readline()  # Skip past PORO's data line entirely.
        # Now positioned right at "PERMX" - reading PORO still succeeds
        # even though it is "behind" the current position.
        kw = ResdataKW.read_grdecl(f, "PORO", rd_type=ResDataType.RD_FLOAT)
    assert list(kw.numpy_view()) == pytest.approx([0.1, 0.2, 0.3])


def test_that_read_grdecl_advances_position_past_the_terminator_for_next_read(
    tmp_path,
):
    path = write_grdecl_file(tmp_path, "PORO\n0.1 0.2 0.3 /\nPERMX\n10 20 30 /\n")
    with open(path) as f:
        poro = ResdataKW.read_grdecl(f, "PORO", rd_type=ResDataType.RD_FLOAT)
        permx = ResdataKW.read_grdecl(f, "PERMX", rd_type=ResDataType.RD_FLOAT)
    assert list(poro.numpy_view()) == pytest.approx([0.1, 0.2, 0.3])
    assert list(permx.numpy_view()) == pytest.approx([10, 20, 30])
