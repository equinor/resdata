from resdata.util.util import monkey_the_camel
from .rd_kw import ResdataKW


class Resdata3DKW(ResdataKW):
    """
    Class for working with Eclipse keywords defined over a grid

    The Resdata3DKW class is derived from the ResdataKW class, and most of the
    methods are implemented in the ResdataKW base class. The purpose of
    the Resdata3DKW class is to simplify working with 3D properties like
    PORO or SATNUM.

    The Resdata3DKW class has an attached Grid which is used to support
    [i,j,k] indexing, and a defined default value which is used when
    reading an inactive value. The Resdata3DKW keyword instances are
    returned from the ResdataInitFile and ResdataRestartFile classes, in
    addition you can excplicitly \"cast\" a ResdataKW keyword to Resdata3DKW
    with the Resdata3DKW.castFromKW() classmethod.

    Usage example:

       from resdata.ecl import ResdataInitFile,Grid

       >>> grid = Grid("CASE.EGRID")
       >>> file = ResdataInitFile(grid, "CASE.INIT")

       >>> permx_kw = file["PORO"][0]
       >>> porv_kw = file["PORV"][0]

       >>> permx_kw.setDefault( -1 )
       >>> for k in range(grid.getNZ()):
       >>>     for j in range(grid.getNY()):
       >>>         for i in range(grid.getNX()):
       >>>             print('"(%d,%d,%d)  Permx:%g  Porv:%g"' % (i,j,k,permx_kw[i,j,k] , porv_kw[i,j,k]))

    In the example we open an INIT file and extract the PERMX
    and PORV properties, and then iterate over all the cells in the
    grid.

    In the INIT file the PORV keyword is stored with all cells,
    whereas the PERMX keyword typically only has the active cells
    stored, this active/inactive gymnastics is handled
    transparently. With the call:

        permx_kw.setDefault( -1 )

    we say that we want the value -1 for all inactive cells in the
    PERMX property.

    """

    def __init__(self, kw, grid, value_type, default_value=0, global_active=False):
        if global_active:
            size = grid.getGlobalSize()
        else:
            size = grid.getNumActive()
        super(Resdata3DKW, self).__init__(kw, size, value_type)
        self.grid = grid
        self.global_active = global_active
        self.setDefault(default_value)

    @classmethod
    def create(cls, kw, grid, value_type, default_value=0, global_active=False):
        new_kw = Resdata3DKW(kw, grid, value_type, default_value, global_active)
        return new_kw

    @classmethod
    def read_grdecl(cls, grid, fileH, kw, strict=True, rd_type=None):
        """
        Will load an Resdata3DKW instance from a grdecl formatted filehandle.

        See the base class ResdataKW.read_grdecl() for more documentation.
        """
        kw = super(Resdata3DKW, cls).read_grdecl(fileH, kw, strict, rd_type)
        Resdata3DKW.castFromKW(kw, grid)
        return kw

    def __getitem__(self, index):
        """Will return item [g] or [i,j,k].

        The __getitem__() methods supports both scalar indexing like
        [g] and tuples [i,j,k]. If the input argument is given as a
        [i,j,k] tuple it is converted to an active index before the
        final lookup.

        If the [i,j,k] input corresponds to an inactive cell in a
        keyword with only nactive elements the default value will be
        returned. By default the default value will be 0, but another
        value can be assigned with the setDefault() method.
        """
        if isinstance(index, tuple):
            global_index = self.grid.get_global_index(ijk=index)
            if self.global_active:
                index = global_index
            else:
                if not self.grid.active(global_index=global_index):
                    return self.getDefault()
                else:
                    index = self.grid.get_active_index(ijk=index)

        return super(Resdata3DKW, self).__getitem__(index)

    def __setitem__(self, index, value):
        """Set the value of at index [g] or [i,j,k].

        The __setitem__() methods supports both scalar indexing like
        [g] and tuples [i,j,k]. If the input argument is given as a
        [i,j,k] tuple it is converted to an active index before the
        final assignment.

        If you try to assign an inactive cell in a keyword with only
        nactive elements a ValueError() exception will be raised.
        """
        if isinstance(index, tuple):
            global_index = self.grid.get_global_index(ijk=index)
            if self.global_active:
                index = global_index
            else:
                if not self.grid.active(global_index=global_index):
                    raise ValueError(
                        "Tried to assign value to inactive cell: (%d,%d,%d)" % index
                    )
                else:
                    index = self.grid.get_active_index(ijk=index)

        return super(Resdata3DKW, self).__setitem__(index, value)

    @classmethod
    def cast_from_kw(cls, kw, grid, default_value=0):
        """Will convert a normal ResdataKW to a Resdata3DKW.

        The method will convert a normal ResdataKW instance to Resdata3DKw
        instance with an attached grid and a default value.

        The method will check that size of the keyword is compatible
        with the grid dimensions, i.e. the keyword must have either
        nactive or nx*ny*nz elements. If the size of the keyword is
        not compatible with the grid dimensions a ValueError exception
        is raised.

        Example:

          1. Load the poro keyword from a grdecl formatted file.
          2. Convert the keyword to a 3D keyword.


        from resdata.ecl import Grid,ResdataKW,Resdata3DKW

        >>> grid = Grid("CASE.EGRID")
        >>> poro = ResdataKW.read_grdecl(open("poro.grdecl") , "PORO")
        >>> Resdata3DKW.castFromKW( poro , grid )

        >>> print('Porosity in cell (10,11,12):%g' % poro[10,11,12])
        """
        if len(kw) == grid.getGlobalSize():
            kw.global_active = True
        elif len(kw) == grid.getNumActive():
            kw.global_active = False
        else:
            raise ValueError(
                "Size mismatch - must have size matching global/active size of grid"
            )

        kw.__class__ = cls
        kw.default_value = default_value
        kw.grid = grid
        if len(kw) == grid.getGlobalSize():
            kw.global_active = True
        else:
            kw.global_active = False

        kw.setDefault(default_value)
        return kw

    def compressed_copy(self):
        """Will return a ResdataKW copy with nactive elements.

        The returned copy will be of type ResdataKW; i.e. no default
        interpolation and only linear access in the [] operator. The
        main purpose of this is to facilitate iteration over the
        active index, and for writing binary files.
        """
        return self.grid.compressedKWCopy(self)

    def global_copy(self):
        """Will return a ResdataKW copy with nx*ny*nz elements.

        The returned copy will be of type ResdataKW; i.e. no default
        interpolation and only linear access in the [] operator. The
        main purpose of this is to facilitate iteration over the
        global index, and for writing binary files.
        """
        return self.grid.globalKWCopy(self, self.getDefault())

    def dims(self):
        return (self.grid.getNX(), self.grid.getNY(), self.grid.getNZ())

    def set_default(self, default_value):
        self.default_value = default_value

    def get_default(self):
        return self.default_value


monkey_the_camel(Resdata3DKW, "castFromKW", Resdata3DKW.cast_from_kw, classmethod)
monkey_the_camel(Resdata3DKW, "compressedCopy", Resdata3DKW.compressed_copy)
monkey_the_camel(Resdata3DKW, "globalCopy", Resdata3DKW.global_copy)
monkey_the_camel(Resdata3DKW, "setDefault", Resdata3DKW.set_default)
monkey_the_camel(Resdata3DKW, "getDefault", Resdata3DKW.get_default)
