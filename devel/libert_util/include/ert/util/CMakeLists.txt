set(header_files)
foreach (type int double bool long time_t size_t float)
  set(TYPE ${type} )      
  set(header_target ${type}_vector.h)    

  configure_file( vector_template.h ${header_target})

  list( APPEND header_files ${CMAKE_CURRENT_BINARY_DIR}/${header_target} )
endforeach( type )

foreach(header ${header_files})
    install(FILES ${header} DESTINATION ${CMAKE_INSTALL_PREFIX}/include/ert/util)
endforeach()




