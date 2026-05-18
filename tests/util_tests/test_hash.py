from ctypes import c_void_p

from tests import ResdataTest
from resdata.util.util import Hash, StringHash, DoubleHash, IntegerHash


class HashTest(ResdataTest):
    def test_string_hash(self):
        _hash = StringHash()

        self.assertEqual(len(_hash), 0)

        _hash["hipp"] = ""

        self.assertEqual(len(_hash), 1)

        with self.assertRaises(ValueError):
            _hash["hopp"] = 55

        with self.assertRaises(KeyError):
            hopp = _hash["hopp"]

        self.assertTrue("hipp" in _hash)

        self.assertEqual(list(_hash.keys()), ["hipp"])

    def test_int_hash(self):
        _hash = IntegerHash()

        with self.assertRaises(ValueError):
            _hash["one"] = "ein"

        with self.assertRaises(ValueError):
            _hash["one"] = 1.0

        _hash["two"] = 2

        self.assertEqual(_hash["two"], 2)

    def test_double_hash(self):
        _hash = DoubleHash()

        with self.assertRaises(ValueError):
            _hash["one"] = "ein"

        _hash["two"] = 2
        _hash["three"] = 3.0

        self.assertEqual(_hash["two"], 2)
        self.assertEqual(_hash["three"], 3.0)

    def test_c_void_p_hash(self):
        _hash = Hash()

        cp = c_void_p(512)
        _hash["1"] = cp

        self.assertEqual(_hash["1"], cp.value)

    def test_for_in_hash(self):
        _hash = StringHash()

        _hash["one"] = "one"
        _hash["two"] = "two"
        _hash["three"] = "three"

        for key in _hash:
            self.assertTrue(key in _hash)
