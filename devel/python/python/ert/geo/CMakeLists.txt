set(PYTHON_SOURCES
    __init__.py
    geo_polygon.py
    polyline.py
    xyz_reader.py
)

add_python_package("python.ert.geo"  ${PYTHON_INSTALL_PREFIX}/ert/geo "${PYTHON_SOURCES}" True)

