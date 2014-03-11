from   ert.cwrap import CWrapper , CWrapperNameSpace
from   ert.well import WELL_LIB


class WellTS:
    
    def __init__(self):
        pass

    


# 2. Creating a wrapper object around the libecl library, 
cwrapper = CWrapper( WELL_LIB )
cwrapper.registerType( "well_ts" , WellTS )


# 3. Installing the c-functions used to manipulate ecl_file instances.
#    These functions are used when implementing the EclFile class, not
#    used outside this scope.
cfunc = CWrapperNameSpace("well_ts")


