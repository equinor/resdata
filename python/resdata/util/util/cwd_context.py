import os

from typing_extensions import deprecated


@deprecated(
    "CWDContext is deprecated and will be removed in version 7. Use contextlib.chdir"
)
class CWDContext:
    def __init__(self, path):
        self.cwd = os.getcwd()
        if os.path.isdir(path):
            os.chdir(path)
        else:
            raise OSError("Path:%s does not exist" % path)

    def __exit__(self, exc_type, exc_val, exc_tb):
        os.chdir(self.cwd)
        return False

    def __enter__(self):
        return self
