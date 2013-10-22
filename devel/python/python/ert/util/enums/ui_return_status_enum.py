from ert.cwrap import BaseCEnum
from ert.util import UTIL_LIB


class UIReturnStatusEnum(BaseCEnum):
    UI_RETURN_OK = None
    UI_RETURN_FAIL = None

UIReturnStatusEnum.addEnum( "UI_RETURN_OK" , 1 )
UIReturnStatusEnum.addEnum( "UI_RETURN_FAIL" , 2 )
UIReturnStatusEnum.registerEnum(UTIL_LIB , "ui_return_type_enum_iget")
