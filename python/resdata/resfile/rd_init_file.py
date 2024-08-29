from resdata import FileMode, FileType
from resdata.resfile import Resdata3DFile, ResdataFile


class ResdataInitFile(Resdata3DFile):
    def __init__(self, grid, filename, flags=FileMode.DEFAULT):
        file_type, _, _ = ResdataFile.getFileType(filename)
        if file_type == FileType.INIT:
            super().__init__(grid, filename, flags)
        else:
            err = (
                f'The input filename "{filename}" does not correspond to an init file.'
            )
            err += "  Please follow the Eclipse naming conventions."
            raise ValueError(err)
