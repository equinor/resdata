from cwrap import BaseCClass
from resdata.util.util import monkey_the_camel
from resdata.util.util import IntVector
from resdata import ResdataPrototype


class Layer(BaseCClass):
    TYPE_NAME = "rd_layer"
    _alloc = ResdataPrototype("void* layer_alloc(int,  int)", bind=False)
    _free = ResdataPrototype("void layer_free(rd_layer)")
    _get_nx = ResdataPrototype("int layer_get_nx(rd_layer)")
    _get_ny = ResdataPrototype("int layer_get_ny(rd_layer)")
    _set_cell = ResdataPrototype("void layer_iset_cell_value(rd_layer, int, int, int)")
    _get_cell = ResdataPrototype("int layer_iget_cell_value(rd_layer, int, int)")
    _get_bottom_barrier = ResdataPrototype(
        "bool layer_iget_bottom_barrier(rd_layer, int, int)"
    )
    _get_left_barrier = ResdataPrototype(
        "bool layer_iget_left_barrier(rd_layer, int, int)"
    )
    _cell_contact = ResdataPrototype(
        "bool layer_cell_contact(rd_layer, int, int, int, int)"
    )
    _add_barrier = ResdataPrototype("void layer_add_barrier(rd_layer, int, int)")
    _add_ijbarrier = ResdataPrototype(
        "void layer_add_ijbarrier(rd_layer, int, int, int, int)"
    )
    _add_interp_barrier = ResdataPrototype(
        "void layer_add_interp_barrier(rd_layer, int, int)"
    )
    _clear_cells = ResdataPrototype("void layer_clear_cells(rd_layer)")
    _assign = ResdataPrototype("void layer_assign(rd_layer, int)")
    _cell_sum = ResdataPrototype("int layer_get_cell_sum(rd_layer)")
    _update_connected = ResdataPrototype(
        "void layer_update_connected_cells(rd_layer,int,int,int,int)"
    )
    _cells_equal = ResdataPrototype(
        "void layer_cells_equal(rd_layer, int,rd_int_vector,rd_int_vector)"
    )
    _count_equal = ResdataPrototype("int layer_count_equal(rd_layer, int)")
    _active_cell = ResdataPrototype("bool layer_iget_active(rd_layer, int,int)")
    _update_active = ResdataPrototype(
        "bool layer_update_active(rd_layer, rd_grid, int)"
    )

    def __init__(self, nx, ny):
        c_ptr = self._alloc(nx, ny)
        if c_ptr:
            super().__init__(c_ptr)
        else:
            raise ValueError("Invalid input - no Layer object created")

    def _assert_ij(self, i, j):
        if i < 0 or i >= self.get_nx():
            raise ValueError(f"Invalid layer i:{i}")

        if j < 0 or j >= self.get_ny():
            raise ValueError(f"Invalid layer j:{j}")

    def __unpack_index(self, index):
        try:
            (i, j) = index
        except TypeError as err:
            raise ValueError(
                f"Index:{str(index)} is invalid - must have two integers"
            ) from err

        self._assert_ij(i, j)

        return (i, j)

    def __setitem__(self, index, value):
        (i, j) = self.__unpack_index(index)
        self._set_cell(i, j, value)

    def active_cell(self, i, j):
        self._assert_ij(i, j)
        return self._active_cell(i, j)

    def update_active(self, grid, k):
        if grid.getNX() != self.getNX():
            raise ValueError(
                "NX dimension mismatch. Grid:%d  layer:%d"
                % (grid.getNX(), self.getNX())
            )

        if grid.getNY() != self.getNY():
            raise ValueError(
                "NY dimension mismatch. Grid:%d  layer:%d"
                % (grid.getNY(), self.getNY())
            )

        if k >= grid.getNZ():
            raise ValueError("K value invalid: Grid range [0,%d)" % grid.getNZ())

        self._update_active(grid, k)

    def __getitem__(self, index):
        (i, j) = self.__unpack_index(index)
        return self._get_cell(i, j)

    def bottom_barrier(self, i, j):
        self._assert_ij(i, j)
        return self._get_bottom_barrier(i, j)

    def left_barrier(self, i, j):
        self._assert_ij(i, j)
        return self._get_left_barrier(i, j)

    def get_nx(self):
        return self._get_nx()

    @property
    def nx(self):
        return self._get_nx()

    def get_ny(self):
        return self._get_ny()

    @property
    def ny(self):
        return self._get_ny()

    def free(self):
        self._free()

    def cell_contact(self, p1, p2):
        i1, j1 = p1
        i2, j2 = p2

        if not 0 <= i1 < self.getNX():
            raise IndexError("Invalid i1:%d" % i1)

        if not 0 <= i2 < self.getNX():
            raise IndexError("Invalid i2:%d" % i2)

        if not 0 <= j1 < self.getNY():
            raise IndexError("Invalid i1:%d" % j1)

        if not 0 <= j2 < self.getNY():
            raise IndexError("Invalid i2:%d" % j2)

        return self._cell_contact(i1, j1, i2, j2)

    def add_interp_barrier(self, c1, c2):
        self._add_interp_barrier(c1, c2)

    def add_polyline_barrier(self, polyline, grid, k):
        if len(polyline) > 1:
            for i in range(len(polyline) - 1):
                x1, y1 = polyline[i]
                x2, y2 = polyline[i + 1]

                c1 = grid.findCellCornerXY(x1, y1, k)
                c2 = grid.findCellCornerXY(x2, y2, k)

                self.addInterpBarrier(c1, c2)

    def add_fault_barrier(self, fault, K, link_segments=True):
        fault_layer = fault[K]
        num_lines = len(fault_layer)
        for index, fault_line in enumerate(fault_layer):
            for segment in fault_line:
                c1, c2 = segment.getCorners()
                self._add_barrier(c1, c2)

            if index < num_lines - 1:
                next_line = fault_layer[index + 1]
                next_segment = next_line[0]
                next_c1, _ = next_segment.getCorners()

                if link_segments:
                    self.addInterpBarrier(c2, next_c1)

    def add_ij_barrier(self, ij_list):
        if len(ij_list) < 2:
            raise ValueError("Must have at least two (i,j) points")

        nx = self.get_nx()
        ny = self.get_ny()
        p1 = ij_list[0]
        i1, j1 = p1
        for p2 in ij_list[1:]:
            i2, j2 = p2
            if i1 == i2 or j1 == j2:
                if not 0 <= i2 <= nx:
                    raise ValueError(
                        "i value:%d invalid. Valid range: [0,%d] " % (i1, i2)
                    )

                if not 0 <= j2 <= ny:
                    raise ValueError(
                        "i value:%d invalid. Valid range: [0,%d] " % (j1, j2)
                    )

                self._add_ijbarrier(i1, j1, i2, j2)
                p1 = p2
                i1, j1 = p1
            else:
                raise ValueError("Must have i1 == i2 or j1 == j2")

    def cell_sum(self):
        return self._cell_sum()

    def clear_cells(self):
        """
        Will reset all cell and edge values to zero. Barriers will be left
        unchanged.
        """
        self._clear_cells()

    def assign(self, value):
        """
        Will set the cell value to @value in all cells. Barriers will not be changed
        """
        self._assign(value)

    def update_connected(self, ij, new_value, org_value=None):
        """
        Will update cell value of all cells in contact with cell ij to the
        value @new_value. If org_value is not supplied, the current
        value in cell ij is used.
        """
        if org_value is None:
            org_value = self[ij]

        if self[ij] == org_value:
            self._update_connected(ij[0], ij[1], org_value, new_value)
        else:
            raise ValueError("Cell %s is not equal to %d \n" % (ij, org_value))

    def cells_equal(self, value):
        """
        Will return a list [(i1,j1),(i2,j2), ...(in,jn)] of all cells with value @value.
        """
        i_list = IntVector()
        j_list = IntVector()
        self._cells_equal(value, i_list, j_list)
        ij_list = []
        for i, j in zip(i_list, j_list):
            ij_list.append((i, j))
        return ij_list

    def count_equal(self, value):
        return self._count_equal(value)


monkey_the_camel(Layer, "activeCell", Layer.active_cell)
monkey_the_camel(Layer, "updateActive", Layer.update_active)
monkey_the_camel(Layer, "bottomBarrier", Layer.bottom_barrier)
monkey_the_camel(Layer, "leftBarrier", Layer.left_barrier)
monkey_the_camel(Layer, "getNX", Layer.get_nx)
monkey_the_camel(Layer, "getNY", Layer.get_ny)
monkey_the_camel(Layer, "cellContact", Layer.cell_contact)
monkey_the_camel(Layer, "addInterpBarrier", Layer.add_interp_barrier)
monkey_the_camel(Layer, "addPolylineBarrier", Layer.add_polyline_barrier)
monkey_the_camel(Layer, "addFaultBarrier", Layer.add_fault_barrier)
monkey_the_camel(Layer, "addIJBarrier", Layer.add_ij_barrier)
monkey_the_camel(Layer, "cellSum", Layer.cell_sum)
monkey_the_camel(Layer, "clearCells", Layer.clear_cells)
monkey_the_camel(Layer, "updateConnected", Layer.update_connected)
monkey_the_camel(Layer, "cellsEqual", Layer.cells_equal)
monkey_the_camel(Layer, "countEqual", Layer.count_equal)
