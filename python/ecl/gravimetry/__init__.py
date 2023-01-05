"""
  ecl_grav/EclGrav: Class used to simplify evaluation of ECLIPSE
     modelling time-lapse gravitational surveys.

  ecl_subsidence/EclSubsidence: Small class used to evaluate simulated
     subsidence from ECLIPSE simulations; analogous to the EcLGrav
     functionality.
"""

from .ecl_subsidence import EclSubsidence
from .ecl_grav_calc import phase_deltag, deltag
from .ecl_grav import EclGrav
