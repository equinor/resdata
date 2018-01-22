import sys
if not 'ecl.grid' in sys.modules:
  raise ImportError("ecl.grid must be imported before ecl.faults.")

from .layer import Layer
from .fault_collection import FaultCollection
from .fault import Fault
from .fault_line import FaultLine
from .fault_segments import FaultSegment , SegmentMap
from .fault_block import FaultBlock , FaultBlockCell
from .fault_block_layer import FaultBlockLayer
