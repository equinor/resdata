find_package(Sphinx REQUIRED)

set(DOC_INSTALL_PREFIX "share/resdata/docs")
set(doc_build "${PROJECT_BINARY_DIR}/doc_build")
file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/code" DESTINATION ${doc_build})

configure_file(index.rst.in "${doc_build}/index.rst")
configure_file(conf.py.in "${doc_build}/conf.py")

add_custom_target(
  api-doc ALL
  COMMAND sphinx-apidoc -e -o "${doc_build}/api"
          ${PROJECT_BINARY_DIR}/${PYTHON_INSTALL_PREFIX} tests
  WORKING_DIRECTORY ${PROJECT_BINARY_DIR}/${PYTHON_INSTALL_PREFIX}
  DEPENDS resdata)

add_custom_target(
  html-doc ALL
  COMMAND sphinx-build -b html -d ${doc_build}/doctrees ${doc_build}
          ${doc_build}/html
  WORKING_DIRECTORY ${PROJECT_BINARY_DIR}/${PYTHON_INSTALL_PREFIX}
  DEPENDS api-doc)

install(DIRECTORY ${doc_build}/html
        DESTINATION ${CMAKE_INSTALL_PREFIX}/${DOC_INSTALL_PREFIX})
