from resdata.resfile import Resdata3DKW, ResdataFile


class Resdata3DFile(ResdataFile):
    def __init__(self, grid, filename, flags=0):
        self.grid = grid
        super().__init__(filename, flags)

    def __getitem__(self, index):
        return_arg = super().__getitem__(index)
        kw_list = return_arg if isinstance(return_arg, list) else [return_arg]

        # Go through all the keywords and try inplace promotion to Resdata3DKW
        for kw in kw_list:
            try:  # noqa: SIM105
                Resdata3DKW.castFromKW(kw, self.grid)
            except ValueError:
                pass

        return return_arg
