import os
from ecl.test import TestAreaContext
from tests import EclTest
from ecl.util import PathFormat

class PathFmtTest(EclTest):

    def test_create(self):
        path_fmt = PathFormat("random/path/%d-%d")
        self.assertIn('random/path', repr(path_fmt))
        self.assertTrue(str(path_fmt).startswith('PathFormat('))
