from cwrap import BaseCEnum


class RngAlgTypeEnum(BaseCEnum):
    TYPE_NAME = "rd_rng_alg_type_enum"
    MZRAN = None


RngAlgTypeEnum.addEnum("MZRAN", 1)
