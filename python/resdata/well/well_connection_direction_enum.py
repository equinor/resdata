from cwrap import BaseCEnum


class WellConnectionDirection(BaseCEnum):
    TYPE_NAME = "well_connection_dir_enum"
    well_conn_dirX = None
    well_conn_dirY = None
    well_conn_dirZ = None
    well_conn_fracX = None
    well_conn_fracY = None


WellConnectionDirection.addEnum("well_conn_dirX", 1)
WellConnectionDirection.addEnum("well_conn_dirY", 2)
WellConnectionDirection.addEnum("well_conn_dirZ", 3)
WellConnectionDirection.addEnum("well_conn_fracX", 4)
WellConnectionDirection.addEnum("well_conn_fracY", 5)
