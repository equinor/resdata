set(PYTHON_SOURCES
    __init__.py
    history.py
    history_source_enum.py
    sched_file.py
)

add_python_package("python.ecl.sched"  ${PYTHON_INSTALL_PREFIX}/ecl/sched "${PYTHON_SOURCES}" True)

