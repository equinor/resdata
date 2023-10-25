from cwrap import BaseCEnum


class SummaryVarType(BaseCEnum):
    TYPE_NAME = "rd_sum_var_type"
    RD_SMSPEC_INVALID_VAR = None
    RD_SMSPEC_FIELD_VAR = None
    RD_SMSPEC_REGION_VAR = None
    RD_SMSPEC_GROUP_VAR = None
    RD_SMSPEC_WELL_VAR = None
    RD_SMSPEC_SEGMENT_VAR = None
    RD_SMSPEC_BLOCK_VAR = None
    RD_SMSPEC_AQUIFER_VAR = None
    RD_SMSPEC_COMPLETION_VAR = None
    RD_SMSPEC_NETWORK_VAR = None
    RD_SMSPEC_REGION_2_REGION_VAR = None
    RD_SMSPEC_LOCAL_BLOCK_VAR = None
    RD_SMSPEC_LOCAL_COMPLETION_VAR = None
    RD_SMSPEC_LOCAL_WELL_VAR = None
    RD_SMSPEC_MISC_VAR = None


SummaryVarType.addEnum("RD_SMSPEC_INVALID_VAR", 0)
SummaryVarType.addEnum("RD_SMSPEC_FIELD_VAR", 1)
SummaryVarType.addEnum("RD_SMSPEC_REGION_VAR", 2)
SummaryVarType.addEnum("RD_SMSPEC_GROUP_VAR", 3)
SummaryVarType.addEnum("RD_SMSPEC_WELL_VAR", 4)
SummaryVarType.addEnum("RD_SMSPEC_SEGMENT_VAR", 5)
SummaryVarType.addEnum("RD_SMSPEC_BLOCK_VAR", 6)
SummaryVarType.addEnum("RD_SMSPEC_AQUIFER_VAR", 7)
SummaryVarType.addEnum("RD_SMSPEC_COMPLETION_VAR", 8)
SummaryVarType.addEnum("RD_SMSPEC_NETWORK_VAR", 9)
SummaryVarType.addEnum("RD_SMSPEC_REGION_2_REGION_VAR", 10)
SummaryVarType.addEnum("RD_SMSPEC_LOCAL_BLOCK_VAR", 11)
SummaryVarType.addEnum("RD_SMSPEC_LOCAL_COMPLETION_VAR", 12)
SummaryVarType.addEnum("RD_SMSPEC_LOCAL_WELL_VAR", 13)
SummaryVarType.addEnum("RD_SMSPEC_MISC_VAR", 14)
