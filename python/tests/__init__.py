import os.path
from ecl.test import ExtendedTestCase


def source_root():
    path_list = os.path.dirname(os.path.abspath(__file__)).split("/")
    while True:
        git_path = os.path.join(os.sep, "/".join(path_list), ".git")
        if os.path.isdir(git_path):
            return os.path.join(os.sep, *path_list)
        path_list.pop()


class EclTest(ExtendedTestCase):
    SOURCE_ROOT = source_root()
    TESTDATA_ROOT = os.path.join(SOURCE_ROOT, "test-data")

