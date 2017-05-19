project(libecl-utilxx CXX)

if (NOT ERT_BUILD_CXX)
    return ()
endif ()

add_library(ert_utilxx src/TestArea.cpp)
target_link_libraries(ert_utilxx ert_util)
target_include_directories(ert_util
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
           $<INSTALL_INTERFACE:include>
    )

install(DIRECTORY include/ DESTINATION include)
install(TARGETS ert_utilxx
        EXPORT  ecl-config
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

set_target_properties(ert_utilxx PROPERTIES
                        VERSION ${ERT_VERSION_MAJOR}.${ERT_VERSION_MINOR}
                        SOVERSION ${ERT_VERSION_MAJOR}
                     )

if (NOT BUILD_TESTS)
    return ()
endif()

foreach (test ert_util_unique_ptr ert_util_test_area_xx)
    add_executable(${test} tests/${test}.cpp)
    target_link_libraries(${test} ert_utilxx)
    add_test(NAME ${test} COMMAND ${test})
endforeach()
