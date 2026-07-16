from resdata.rd_util import FileMode

from .rd_file import ResdataFile


class ResdataFileContextManager:
    def __init__(self, rd_file):
        self.__rd_file = rd_file

    def __enter__(self):
        return self.__rd_file

    def __exit__(self, exc_type, exc_val, exc_tb):
        self.__rd_file.close()
        return False


def open_rd_file(file_name, flags=FileMode.DEFAULT):
    return ResdataFileContextManager(ResdataFile(file_name, flags))
