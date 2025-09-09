project(resdata-applications)

function(target_link_resdata target)
  target_link_libraries(${target} resdata)
  if(SKBUILD)
    set_target_properties(${target} PROPERTIES INSTALL_RPATH "$ORIGIN/../.libs")
  endif()
endfunction()

if(BUILD_APPLICATIONS)
  add_executable(sum_write resdata/sum_write.cpp)
  add_executable(make_grid resdata/make_grid.c)
  add_executable(summary resdata/view_summary.cpp)
  add_executable(kw_extract resdata/kw_extract.cpp)
  target_link_resdata(sum_write)
  target_link_resdata(make_grid)
  target_link_resdata(summary)
  target_link_resdata(kw_extract)

  list(APPEND apps make_grid summary kw_extract)

  foreach(app rd_pack rd_unpack grid_info select_test load_test)
    add_executable(${app} resdata/${app}.c)
    target_link_resdata(${app})
    list(APPEND apps ${app})
    if(ERT_LINUX)
      # The .x extension creates problems on windows
      set_target_properties(${app} PROPERTIES SUFFIX ".x")
    endif()
  endforeach()

  if(ERT_LINUX)
    foreach(app convert grdecl_test kw_list)
      add_executable(${app} resdata/${app}.c)
      target_link_resdata(${app})
      # The .x extension creates problems on windows
      set_target_properties(${app} PROPERTIES SUFFIX ".x")
      list(APPEND apps ${app})
    endforeach()

    set_target_properties(summary PROPERTIES SUFFIX ".x")
  endif()

  add_executable(segment_info well/segment_info.c)

  target_link_resdata(segment_info)

  list(APPEND apps segment_info)
endif()

if(BUILD_RD_SUMMARY)
  add_executable(rd_summary resdata/view_summary.cpp)
  target_link_resdata(rd_summary)
  list(APPEND apps rd_summary)

  install(FILES ${PROJECT_SOURCE_DIR}/man/man1/rd_summary.1
          DESTINATION ${CMAKE_INSTALL_MANDIR}/man1)
endif()

if(apps)
  if(ERT_USE_OPENMP)
    foreach(app ${apps})
      target_compile_options(${app} PUBLIC ${OpenMP_C_FLAGS})
      set_property(
        TARGET ${app}
        APPEND
        PROPERTY LINK_FLAGS ${OpenMP_C_FLAGS})
    endforeach()
  endif()
  install(
    TARGETS ${apps}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()
