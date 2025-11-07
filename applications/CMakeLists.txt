project(resdata-applications)

function(target_link_resdata target)
  target_link_libraries(${target} resdata)
  if(SKBUILD)
    set_target_properties(${target} PROPERTIES INSTALL_RPATH "$ORIGIN/../.libs")
  endif()
endfunction()

if(BUILD_APPLICATIONS)
  add_executable(sum_write resdata/sum_write.cpp)
  target_link_resdata(sum_write)

  foreach(app rd_pack rd_unpack load_test)
    add_executable(${app} resdata/${app}.cpp)
    target_link_resdata(${app})
    list(APPEND apps ${app})
    if(ERT_LINUX)
      # The .x extension creates problems on windows
      set_target_properties(${app} PROPERTIES SUFFIX ".x")
    endif()
  endforeach()

  if(ERT_LINUX)
    foreach(app convert grdecl_test kw_list)
      add_executable(${app} resdata/${app}.cpp)
      target_link_resdata(${app})
      # The .x extension creates problems on windows
      set_target_properties(${app} PROPERTIES SUFFIX ".x")
      list(APPEND apps ${app})
    endforeach()

  endif()
endif()

if(apps)
  if(ERT_USE_OPENMP)
    foreach(app ${apps})
      target_compile_options(${app} PUBLIC ${OpenMP_CXX_FLAGS})
      set_property(
        TARGET ${app}
        APPEND
        PROPERTY LINK_FLAGS ${OpenMP_CXX_FLAGS})
    endforeach()
  endif()
  install(
    TARGETS ${apps}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()
