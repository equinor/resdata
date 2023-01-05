from ecl import EclFileEnum, EclFileFlagEnum
from ecl.eclfile import Ecl3DFile, EclFile

ECL_FILE_DEFAULT = EclFileFlagEnum.ECL_FILE_DEFAULT


class EclInitFile(Ecl3DFile):
    def __init__(self, grid, filename, flags=ECL_FILE_DEFAULT):
        file_type, report_step, fmt_file = EclFile.getFileType(filename)
        if file_type == EclFileEnum.ECL_INIT_FILE:
            super(EclInitFile, self).__init__(grid, filename, flags)
        else:
            err = 'The input filename "%s" does not correspond to an init file.'
            err += "  Please follow the Eclipse naming conventions."
            raise ValueError(err % filename)
