from ert.cwrap import BaseCEnum
from ert.well import ECL_WELL_LIB

class WellTypeEnum(BaseCEnum):
    ERT_UNDOCUMENTED_ZERO = None
    ERT_PRODUCER = None
    ERT_WATER_INJECTOR = None
    ERT_GAS_INJECTOR = None
    ERT_OIL_INJECTOR = None

WellTypeEnum.addEnum("UNDOCUMENTED_ZERO", 0)
WellTypeEnum.addEnum("WELL_PRODUCER", 10)
WellTypeEnum.addEnum("WATER_INJECTOR", 22)
WellTypeEnum.addEnum("GAS_INJECTOR", 21)
WellTypeEnum.addEnum("OIL_INJECTOR", 78)

WellTypeEnum.registerEnum(ECL_WELL_LIB, "well_type_enum")