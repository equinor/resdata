from cwrap import BaseCClass

from resdata.util.util import monkey_the_camel
from resdata import ResdataPrototype


class FaultBlockCollection(BaseCClass):
    TYPE_NAME = "rd_fault_block_collection"
    _alloc = ResdataPrototype("void* fault_block_collection_alloc(rd_grid)", bind=False)
    _free = ResdataPrototype(
        "void  fault_block_collection_free(rd_fault_block_collection)"
    )
    _num_layers = ResdataPrototype(
        "int   fault_block_collection_num_layers(rd_fault_block_collection)"
    )
    _scan_keyword = ResdataPrototype(
        "bool  fault_block_collection_scan_kw(rd_fault_block_collection, rd_kw)"
    )
    _get_layer = ResdataPrototype(
        "rd_fault_block_layer_ref  fault_block_collection_get_layer(rd_fault_block_collection, int)"
    )

    def __init__(self, grid):
        c_ptr = self._alloc(grid)
        if c_ptr:
            super(FaultBlockCollection, self).__init__(c_ptr)
        else:
            raise ValueError("Invalid input - failed to create FaultBlockCollection")

        # The underlying C implementation uses lazy evaluation and
        # needs to hold on to the grid reference. We therefore take
        # references to it here, to protect against premature garbage
        # collection.
        self.grid_ref = grid

    def __len__(self):
        return self._num_layers()

    def __repr__(self):
        return self._create_repr("len=%s" % len(self))

    def __getitem__(self, index):
        """
        @rtype: FaultBlockLayer
        """
        if isinstance(index, int):
            if 0 <= index < len(self):
                return self._get_layer(index).setParent(self)
            else:
                raise IndexError("Index:%d out of range [0,%d)" % (index, len(self)))
        else:
            raise TypeError("Index should be integer type")

    def get_layer(self, k):
        """
        @rtype: FaultBlockLayer
        """
        return self[k]

    def free(self):
        self._free()

    def scan_keyword(self, fault_block_kw):
        ok = self._scan_keyword(fault_block_kw)
        if not ok:
            raise ValueError("The fault block keyword had wrong type/size")


monkey_the_camel(FaultBlockCollection, "getLayer", FaultBlockCollection.get_layer)
monkey_the_camel(FaultBlockCollection, "scanKeyword", FaultBlockCollection.scan_keyword)
