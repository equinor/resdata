import os
from ecl.test import ExtendedTestCase, TestAreaContext
from ecl.util import PathFormat

class PathFmtTest(ExtendedTestCase):

    
    def test_create(self):
        path_fmt = PathFormat("random/path/%d-%d")
            
            
