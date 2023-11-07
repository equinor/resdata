import os
from resdata.util.test import PathContext, TestAreaContext
from tests import ResdataTest


class PathContextTest(ResdataTest):
    def test_error(self):
        with TestAreaContext("pathcontext"):
            # Test failure on creating PathContext with an existing path
            os.makedirs("path/1")
            with self.assertRaises(OSError):
                with PathContext("path/1"):
                    pass

            # Test failure on creating PathContext with an existing file
            with open("path/1/file", "w") as f:
                f.write("xx")
            with self.assertRaises(OSError):
                with PathContext("path/1/file"):
                    pass

    def test_chdir(self):
        with PathContext("/tmp/pc"):
            self.assertEqual(os.path.realpath(os.getcwd()), os.path.realpath("/tmp/pc"))

    def test_cleanup(self):
        with TestAreaContext("pathcontext"):
            os.makedirs("path/1")

            with PathContext("path/1/next/2/level"):
                with open("../../file", "w") as f:
                    f.write("Crap")

            self.assertTrue(os.path.isdir("path/1"))
            self.assertTrue(os.path.isdir("path/1/next"))
            self.assertFalse(os.path.isdir("path/1/next/2"))
