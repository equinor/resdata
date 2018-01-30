set(PYTHON_SOURCES
    __init__.py
    ecl_3dkw.py
    ecl_grav.py
    ecl_grav_calc.py
    ecl_grid.py
    ecl_npv.py
    ecl_region.py
    ecl_rft.py
    ecl_rft_cell.py
    ecl_smspec_node.py
    ecl_subsidence.py
    ecl_sum.py
    ecl_sum_keyword_vector.py
    ecl_sum_node.py
    ecl_sum_tstep.py
    ecl_sum_vector.py
    ecl_sum_keyword_vector.py
    ecl_cmp.py
    ecl_sum_var_type.py
    ecl_grid_generator.py
    cell.py
)

#add_python_package("python.ecl.ecl" ${PYTHON_INSTALL_PREFIX}/ecl/ecl "${PYTHON_SOURCES}" True)

#add_subdirectory(faults)
#add_subdirectory(rft)
