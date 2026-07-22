from cwrap import BaseCClass

import resdata.grid.faults._fault_block as _fault_block
from resdata.geometry import CPolylineCollection, GeometryTools, Polyline
from resdata.util.util import DoubleVector, IntVector


class FaultBlockCell:
    def __init__(self, i, j, k, x, y, z):
        self.i = i
        self.j = j
        self.k = k

        self.x = x
        self.y = y
        self.z = z

    def __str__(self):
        return "(%d,%d)" % (self.i, self.j)


class FaultBlock(BaseCClass):
    TYPE_NAME = "rd_fault_block"

    def __init__(self, *args, **kwargs):
        raise NotImplementedError("Class can not be instantiated directly!")

    def __getitem__(self, index):
        if isinstance(index, int):
            if index < 0:
                index += len(self)

            if 0 <= index < len(self):
                i, j, k, x, y, z = _fault_block._export_cell(self, index)
                return FaultBlockCell(i, j, k, x, y, z)
            else:
                raise IndexError("Index:%d out of range: [0,%d)" % (index, len(self)))
        else:
            raise TypeError("Index:%s wrong type - integer expected")

    def __str__(self):
        return "Block ID: %d" % self.get_block_id()

    def __len__(self):
        return _fault_block._get_size(self)

    def free(self):
        _fault_block._free(self)

    def get_centroid(self):
        xc = _fault_block._get_xc(self)
        yc = _fault_block._get_yc(self)
        return (xc, yc)

    def count_inside(self, polygon):
        """
        Will count the number of points in block which are inside polygon.
        """
        inside = 0
        for p in self:
            if GeometryTools.pointInPolygon((p.x, p.y), polygon):
                inside += 1

        return inside

    def get_block_id(self):
        return _fault_block._get_block_id(self)

    def assign_to_region(self, region_id):
        _fault_block._assign_to_region(self, region_id)

    def get_region_list(self):
        regionList = _fault_block._get_region_list(self)
        return regionList.copy()

    def add_cell(self, i, j):
        _fault_block._add_cell(self, i, j)

    def get_global_index_list(self):
        return _fault_block._get_global_index_list(self)

    def get_edge_polygon(self):
        x_list = DoubleVector()
        y_list = DoubleVector()
        cell_list = IntVector()

        _fault_block._trace_edge(self, x_list, y_list, cell_list)
        p = Polyline()
        for x, y in zip(x_list, y_list):
            p.addPoint(x, y)
        return p

    def contains_polyline(self, polyline):
        """
        Will return true if at least one point from the polyline is inside the block.
        """
        edge_polyline = self.get_edge_polygon()
        for p in polyline:
            if GeometryTools.pointInPolygon(p, edge_polyline):
                return True
        else:
            edge_polyline.assertClosed()
            return GeometryTools.polylinesIntersect(edge_polyline, polyline)

    def get_neighbours(self, polylines=None, connected_only=True):
        """
        Will return a list of FaultBlock instances which are in direct
        contact with this block.
        """
        neighbour_id_list = IntVector()
        if polylines is None:
            polylines = CPolylineCollection()

        _fault_block._get_neighbours(self, connected_only, polylines, neighbour_id_list)

        parent_layer = self.get_parent_layer()
        neighbour_list = []
        for _id in neighbour_id_list:
            neighbour_list.append(parent_layer.get_block(_id))
        return neighbour_list

    def get_parent_layer(self):
        return self.parent()
