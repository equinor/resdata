# The structure of the python packages have been reorganized several times,
# unfortunately we have not managed to deprecate properly and guide users over
# to the new layout. We therefor now have two(?) varietes of old layout which
# must(?) be supported for a while.
#
# from ert.ecl import EclEgrid
# from ecl.ecl import EclGrid
#
# Both of these legacy forms are controlled by the cmake switch
# INSTALL_ERT_LEGACY.
#
# EDIT: The ert package has been removed, the ecl.ecl option is still supported.

import warnings

from ecl import EclFileEnum, EclFileFlagEnum, EclPhaseEnum, EclUnitTypeEnum , EclUtil
from ecl import EclTypeEnum, EclDataType
from ecl.summary import EclSumVarType
from ecl.summary import EclSumTStep
from ecl.summary import EclSum #, EclSumVector, EclSumNode, EclSMSPECNode
from ecl.summary import EclSumKeyWordVector
from ecl.rft import EclPLTCell, EclRFTCell
from ecl.rft import EclRFT, EclRFTFile
from ecl.eclfile import FortIO, openFortIO
from ecl.eclfile import EclKW
from ecl.eclfile import Ecl3DKW
from ecl.eclfile import EclFileView
from ecl.eclfile import EclFile , openEclFile
from ecl.eclfile import Ecl3DFile
from ecl.eclfile import EclInitFile
from ecl.eclfile import EclRestartFile
from ecl.grid import EclGrid
from ecl.grid import EclRegion
from ecl.gravimetry import EclSubsidence
from ecl.gravimetry import phase_deltag, deltag
from ecl.gravimetry import EclGrav
from ecl.summary import EclSumNode
from ecl.summary import EclSumVector
from ecl.summary import EclNPV , NPVPriceVector
from ecl.summary import EclCmp
from ecl.grid import EclGridGenerator


warnings.warn(
    "Importing from ecl.ecl is deprecated and will not be available in python3." \
    " For eclipse functionality use \'from ecl import\'.",
    DeprecationWarning
)
