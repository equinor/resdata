from resdata import FileMode, FileType
from resdata.resfile import Resdata3DFile, ResdataFile


class ResdataInitFile(Resdata3DFile):
    def __init__(self, grid, filename, flags=FileMode.DEFAULT):
        file_type, report_step, fmt_file = ResdataFile.getFileType(filename)
        if file_type == FileType.INIT:
            super(ResdataInitFile, self).__init__(grid, filename, flags)
        else:
            err = 'The input filename "%s" does not correspond to an init file.'
            err += "  Please follow the Eclipse naming conventions."
            raise ValueError(err % filename)
