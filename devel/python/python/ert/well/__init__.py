import ert.ecl
import ert.cwrap.clib as clib
WELL_LIB = clib.ert_load("libecl_well")


from .well_state import WellState
from .well_info  import WellInfo
from .well_ts    import WellTS
