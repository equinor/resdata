from ecl.util.test import debug_msg
from tests import EclTest


class DebugTest(EclTest):
    def test_create(self):
        msg = debug_msg("DEBUG")
        self.assertIn(__file__[:-1], msg)
        self.assertIn("DEBUG", msg)
