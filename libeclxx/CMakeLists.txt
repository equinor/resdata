project(libeclxx CXX)

if (NOT ERT_BUILD_CXX)
    return ()
endif ()

add_library(eclxx src/FortIO.cpp
                  src/Smspec.cpp
                  src/EclFilename.cpp
           )

target_link_libraries(eclxx ecl)
target_include_directories(eclxx
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
           $<INSTALL_INTERFACE:include>
    )

install(DIRECTORY include/ DESTINATION include)
install(TARGETS eclxx
        EXPORT  ecl-config
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

set_target_properties(eclxx PROPERTIES
                        VERSION ${ERT_VERSION_MAJOR}.${ERT_VERSION_MINOR}
                        SOVERSION ${ERT_VERSION_MAJOR}
                     )

if (NOT BUILD_TESTS)
    return ()
endif()

foreach (test eclxx_kw eclxx_fortio eclxx_smspec eclxx_filename eclxx_types)
    add_executable(${test} tests/${test}.cpp)
    target_link_libraries(${test} eclxx ert_utilxx)
    add_test(NAME ${test} COMMAND ${test})
endforeach ()
