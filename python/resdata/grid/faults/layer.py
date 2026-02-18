from resdata.util.util import monkey_the_camel

from ._layer import Layer

Layer.TYPE_NAME = "rd_layer"

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
