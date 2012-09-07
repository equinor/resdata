set(source_files rng.c lookup_table.c statistics.c mzran.c set.c hash_node.c hash_sll.c hash.c node_data.c node_ctype.c util.c thread_pool.c msg.c arg_pack.c path_fmt.c menu.c subst_list.c subst_func.c vector.c parser.c stringlist.c matrix.c buffer.c log.c template.c timer.c)


set(header_files ssize_t.h type_macros.h rng.h lookup_table.h statistics.h mzran.h set.h hash.h hash_node.h hash_sll.h node_data.h node_ctype.h util.h thread_pool.h msg.h arg_pack.h path_fmt.h  stringlist.h menu.h subst_list.h subst_func.h vector.h parser.h matrix.h buffer.h log.h template.h timer.h )

if (HAVE_PTHREAD)
  list( APPEND header_files thread_pool_posix.h )
endif()

if (BUILD_LATEX)
   add_definitions( -DBUILD_LATEX )
   add_definitions( -DLATEX_CMD=${LATEX})

   list( APPEND source_files latex.c )
   list( APPEND header_files latex.h )
endif()

if (HAVE_LAPACK)
   list( APPEND source_files ${CMAKE_CURRENT_SOURCE_DIR}/matrix_lapack.c ${CMAKE_CURRENT_SOURCE_DIR}/matrix_blas.c regression.c lars.c stepwise.c)
   list( APPEND header_files ${CMAKE_CURRENT_SOURCE_DIR}/matrix_lapack.h ${CMAKE_CURRENT_SOURCE_DIR}/matrix_blas.h regression.h lars.h stepwise.h)
endif()

# if (!STDBOOL_HEADER)
#   configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/ert_stdbool.h ${CMAKE_CURRENT_BINARY_DIR}/stdbool.h)
#   list( APPEND header_files stdbool.h )
# endif()

foreach (type int double bool long time_t size_t float)
  set(TYPE ${type} )      
  set(src_target        ${CMAKE_CURRENT_BINARY_DIR}/${type}_vector.c)    
  set(header_target     ${CMAKE_CURRENT_BINARY_DIR}/${type}_vector.h)    

  configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/vector_template.c ${src_target})
  configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/vector_template.h ${header_target})

  list( APPEND source_files ${src_target} )
  list( APPEND header_files ${header_target} )
endforeach( type )


# The block_fs filesystem is so heavily dependant on pthreads that it is not 
# built if de not have pthreads.
if (HAVE_PTHREAD)
  list( APPEND source_files ${CMAKE_CURRENT_SOURCE_DIR}/block_fs.c )
  list( APPEND header_files ${CMAKE_CURRENT_SOURCE_DIR}/block_fs.h )
endif()

include_directories( ${CMAKE_CURRENT_BINARY_DIR} )
include_directories( ${CMAKE_CURRENT_SOURCE_DIR} )


set(source_files util.c)
set(header_files util.h)

add_library( util_static  STATIC  ${source_files} )
add_library( util_shared  SHARED  ${source_files} )

if (HAVE_PTHREAD)
  target_link_libraries( util_static ${PTHREAD_LIBRARY} )
  target_link_libraries( util_shared ${PTHREAD_LIBRARY} )
  include_directories( ${PTHREAD_HEADER} )
endif()   

if (HAVE_ZLIB)
  target_link_libraries( util_static ${ZLIB_LIBRARY} )
  target_link_libraries( util_shared ${ZLIB_LIBRARY} )
  include_directories( ${ZLIB_HEADER} )
endif()   

if ( SHLWAPI_LIBRARY )
  target_link_libraries( util_static ${SHWLAPI_LIBRARY} )
  target_link_libraries( util_shared ${SHWLAPI_LIBRARY} )
endif()

if (HAVE_LAPACK)
   target_link_libraries( util_static ${LAPACK_LIBRARY} )
   target_link_libraries( util_shared ${LAPACK_LIBRARY} )
   if (NEED_BLAS)
     target_link_libraries( util_static ${BLAS_LIBRARY} )
     target_link_libraries( util_shared ${BLAS_LIBRARY} )
   endif()
endif()

target_link_libraries( util_static m )
target_link_libraries( util_shared m )

set_target_properties( util_static PROPERTIES OUTPUT_NAME ert_util ARCHIVE_OUTPUT_DIRECTORY /tmp/libutil)
set_target_properties( util_shared PROPERTIES OUTPUT_NAME ert_util LIBRARY_OUTPUT_DIRECTORY /tmp/libutil)

#-----------------------------------------------------------------

if (UTIL_INSTALL_PREFIX)
  install(TARGETS util_static util_shared DESTINATION ${CMAKE_INSTALL_PREFIX}/${UTIL_INSTALL_PREFIX}/lib)
  install(FILES ${header_files} DESTINATION ${CMAKE_INSTALL_PREFIX}/${UTIL_INSTALL_PREFIX}/include)
else()
  install(TARGETS util_static util_shared DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)
  install(FILES ${header_files} DESTINATION ${CMAKE_INSTALL_PREFIX}/include)
endif()

