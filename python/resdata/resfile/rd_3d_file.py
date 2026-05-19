from .rd_3dkw import Resdata3DKW
from .rd_file import ResdataFile


class Resdata3DFile(ResdataFile):
    def __init__(self, grid, filename, flags=0):
        self.grid = grid
        super().__init__(filename, flags)

    def __getitem__(self, index):
        return_arg = super().__getitem__(index)
        if isinstance(return_arg, list):
            kw_list = return_arg
        else:
            kw_list = [return_arg]

        # Go through all the keywords and try inplace promotion to Resdata3DKW
        for kw in kw_list:
            try:
                Resdata3DKW.cast_from_kw(kw, self.grid)
            except ValueError:
                pass

        return return_arg
