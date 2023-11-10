from cwrap import BaseCEnum


class WellType(BaseCEnum):
    TYPE_NAME = "rd_well_type_enum"
    ZERO = None
    PRODUCER = None
    WATER_INJECTOR = None
    GAS_INJECTOR = None
    OIL_INJECTOR = None


WellType.addEnum("ZERO", 0)
WellType.addEnum("PRODUCER", 1)
WellType.addEnum("OIL_INJECTOR", 2)
WellType.addEnum("WATER_INJECTOR", 3)
WellType.addEnum("GAS_INJECTOR", 4)
