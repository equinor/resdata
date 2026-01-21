from hypothesis import given
from resdata.summary import Summary, SummaryVarType

from resfo_utilities import SummaryKeyType, is_rate
from resfo_utilities.testing import summary_variables

DUMMY_NAME = ":+:+:+:+"


def to_ecl(st: SummaryKeyType) -> SummaryVarType | None:
    if st == SummaryKeyType.AQUIFER:
        return SummaryVarType.RD_SMSPEC_AQUIFER_VAR
    if st == SummaryKeyType.BLOCK:
        return SummaryVarType.RD_SMSPEC_BLOCK_VAR
    if st == SummaryKeyType.COMPLETION:
        return SummaryVarType.RD_SMSPEC_COMPLETION_VAR
    if st == SummaryKeyType.FIELD:
        return SummaryVarType.RD_SMSPEC_FIELD_VAR
    if st == SummaryKeyType.GROUP:
        return SummaryVarType.RD_SMSPEC_GROUP_VAR
    if st == SummaryKeyType.LOCAL_BLOCK:
        return SummaryVarType.RD_SMSPEC_LOCAL_BLOCK_VAR
    if st == SummaryKeyType.LOCAL_COMPLETION:
        return SummaryVarType.RD_SMSPEC_LOCAL_COMPLETION_VAR
    if st == SummaryKeyType.LOCAL_WELL:
        return SummaryVarType.RD_SMSPEC_LOCAL_WELL_VAR
    if st == SummaryKeyType.NETWORK:
        return SummaryVarType.RD_SMSPEC_NETWORK_VAR
    if st == SummaryKeyType.SEGMENT:
        return SummaryVarType.RD_SMSPEC_SEGMENT_VAR
    if st == SummaryKeyType.WELL:
        return SummaryVarType.RD_SMSPEC_WELL_VAR
    if st == SummaryKeyType.REGION:
        return SummaryVarType.RD_SMSPEC_REGION_VAR
    if st == SummaryKeyType.INTER_REGION:
        return SummaryVarType.RD_SMSPEC_REGION_2_REGION_VAR
    if st == SummaryKeyType.OTHER:
        return SummaryVarType.RD_SMSPEC_MISC_VAR
    assert False


@given(summary_variables())
def test_that_identify_var_type_is_same_as_resdata(variable):
    assert Summary.var_type(variable) == to_ecl(SummaryKeyType.from_variable(variable))


@given(summary_variables())
def test_rate_determination_is_the_same_as_resdata(variable):
    assert Summary.is_rate(variable) == is_rate(variable)
