set(PYTHON_SOURCES
    __init__.py
    ecl.py
    ecl_case.py
    ecl_default.py
    ecl_file.py
    ecl_grav.py
    ecl_grav_calc.py
    ecl_grid.py
    ecl_kw.py
    ecl_queue.py
    ecl_region.py
    ecl_rft.py
    ecl_rft_cell.py
    ecl_smspec_node.py
    ecl_subsidence.py
    ecl_sum.py
    ecl_sum_node.py
    ecl_sum_vector.py
    ecl_util.py
    fortio.py
)

add_python_package("python.ert.ecl"  ${PYTHON_INSTALL_PREFIX}/ert/ecl "${PYTHON_SOURCES}" True)

