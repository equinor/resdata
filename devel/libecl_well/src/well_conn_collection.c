/*
   Copyright (C) 2013  Statoil ASA, Norway. 
   
   The file 'well_conn_collection.c' is part of ERT - Ensemble based Reservoir Tool. 
    
   ERT is free software: you can redistribute it and/or modify 
   it under the terms of the GNU General Public License as published by 
   the Free Software Foundation, either version 3 of the License, or 
   (at your option) any later version. 
    
   ERT is distributed in the hope that it will be useful, but WITHOUT ANY 
   WARRANTY; without even the implied warranty of MERCHANTABILITY or 
   FITNESS FOR A PARTICULAR PURPOSE.   
    
   See the GNU General Public License at <http://www.gnu.org/licenses/gpl.html> 
   for more details. 
*/

#include <stdbool.h>

#include <ert/util/util.h>
#include <ert/util/vector.h>

#include <ert/ecl/ecl_kw.h>
#include <ert/ecl/ecl_rsthead.h>

#include <ert/ecl_well/well_const.h>
#include <ert/ecl_well/well_conn.h>
#include <ert/ecl_well/well_conn_collection.h>



struct well_conn_collection_struct {
  vector_type * connection_list;
};



well_conn_collection_type * well_conn_collection_alloc() {
  well_conn_collection_type * wellcc = util_malloc( sizeof * wellcc );
  wellcc->connection_list = vector_alloc_new();
  return wellcc;
}

/*
  The collection takes ownership of the connection object and frees it
  when the collection is discarded.  
*/

void well_conn_collection_add( well_conn_collection_type * wellcc , well_conn_type * conn) {
  vector_append_owned_ref( wellcc->connection_list , conn , well_conn_free__);
}

/*
  The collection only stores a refernce to the object, which will be destroyed by 'someone else'. 
*/

void well_conn_collection_add_ref( well_conn_collection_type * wellcc , well_conn_type * conn) {
  vector_append_ref( wellcc->connection_list , conn );
}


void well_conn_collection_free( well_conn_collection_type * wellcc ) {
  vector_free( wellcc->connection_list );
  free( wellcc );
}


int well_conn_collection_get_size( const well_conn_collection_type * wellcc ) {
  return vector_get_size( wellcc->connection_list );
}


const well_conn_type * well_conn_collection_iget_const(const well_conn_collection_type * wellcc , int index) {
  int size = well_conn_collection_get_size( wellcc );
  if (index < size)
    return vector_iget_const( wellcc->connection_list , index );
  else
    return NULL;
}

well_conn_type * well_conn_collection_iget(const well_conn_collection_type * wellcc , int index) {
  int size = well_conn_collection_get_size( wellcc );
  if (index < size)
    return vector_iget( wellcc->connection_list , index );
  else
    return NULL;
}
