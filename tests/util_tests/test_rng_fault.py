from resdata.util.util import RandomNumberGenerator

SEED = "0123456789ABCDEF"
OTHER_SEED = "abcdefghijklmnop"
DEFAULT_SEQUENCE = [1319438365, 1002017353, 1702374509, -211856233, 113582611]
OTHER_DEFAULT_SEQUENCE = [1622601151, 2069016323, 129265198, 1093315151, -734665309]


def seeded_rng(seed=SEED):
    rng = RandomNumberGenerator()
    rng.setState(seed)
    return rng


def default_ints(rng, count):
    return [rng.getInt() for _ in range(count)]


def test_state_size_is_16_and_first_default_int_is_reproducible():
    rng = seeded_rng()
    assert rng.stateSize() == 16
    assert rng.getInt() == DEFAULT_SEQUENCE[0]


def test_seeded_default_get_int_sequence_is_exact():
    assert default_ints(seeded_rng(), 5) == DEFAULT_SEQUENCE


def test_reseeding_restarts_default_get_int_sequence():
    rng = seeded_rng()
    assert default_ints(rng, 3) == DEFAULT_SEQUENCE[:3]
    rng.setState(SEED)
    assert default_ints(rng, 3) == DEFAULT_SEQUENCE[:3]


def test_two_rngs_with_same_seed_produce_same_default_ints():
    rng1 = seeded_rng()
    rng2 = seeded_rng()
    assert default_ints(rng1, 5) == DEFAULT_SEQUENCE
    assert default_ints(rng2, 5) == DEFAULT_SEQUENCE


def test_long_seed_uses_the_first_state_bytes_for_default_ints():
    rng = seeded_rng(SEED + "extra trailing characters")
    assert default_ints(rng, 5) == DEFAULT_SEQUENCE


def test_bytes_seed_matches_string_seed_for_default_ints():
    rng = seeded_rng(b"0123456789ABCDEF")
    assert default_ints(rng, 5) == DEFAULT_SEQUENCE


def test_different_seed_has_its_own_exact_default_int_sequence():
    assert default_ints(seeded_rng(OTHER_SEED), 5) == OTHER_DEFAULT_SEQUENCE


def test_get_double_is_float_in_unit_interval_before_default_int():
    rng = seeded_rng()
    value = rng.getDouble()
    assert type(value) is float
    assert 0.0 <= value < 1.0
    assert rng.getInt() == DEFAULT_SEQUENCE[1]


def test_max_unsigned_int_argument_matches_default_get_int_limit():
    rng = seeded_rng()
    assert rng.getInt(4294967295) == DEFAULT_SEQUENCE[0]


def test_negative_one_argument_matches_default_get_int_limit():
    rng = seeded_rng()
    assert rng.getInt(-1) == DEFAULT_SEQUENCE[0]
