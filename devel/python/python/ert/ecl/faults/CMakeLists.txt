set(PYTHON_SOURCES
    __init__.py
    fault_block.py
    fault_collection.py
    fault.py
    fault_line.py
    fault_segments.py
)

add_python_package("python.ert.ecl.faults"  ${PYTHON_INSTALL_PREFIX}/ert/ecl/faults "${PYTHON_SOURCES}" True)
