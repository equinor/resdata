from ecl.eclfile import EclFile, Ecl3DKW


class Ecl3DFile(EclFile):
    def __init__(self, grid, filename, flags=0):
        self.grid = grid
        super(Ecl3DFile, self).__init__(filename, flags)

    def __getitem__(self, index):
        return_arg = super(Ecl3DFile, self).__getitem__(index)
        if isinstance(return_arg, list):
            kw_list = return_arg
        else:
            kw_list = [return_arg]

        # Go through all the keywords and try inplace promotion to Ecl3DKW
        for kw in kw_list:
            try:
                Ecl3DKW.castFromKW(kw, self.grid)
            except ValueError:
                pass

        return return_arg
