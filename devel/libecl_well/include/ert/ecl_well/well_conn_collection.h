/*
   Copyright (C) 2013  Statoil ASA, Norway. 
   
   The file 'well_conn_collection.h' is part of ERT - Ensemble based Reservoir Tool. 
    
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


#ifndef __WELL_CONN_COLLECTION_H__
#define __WELL_CONN_COLLECTION_H__


#ifdef __cplusplus
extern "C" {
#endif

#include <ert/ecl_well/well_conn.h>

  typedef struct well_conn_collection_struct well_conn_collection_type;

  well_conn_collection_type * well_conn_collection_alloc();
  void                        well_conn_collection_free( well_conn_collection_type * wellcc );
  int                         well_conn_collection_get_size( const well_conn_collection_type * wellcc );
  const well_conn_type *      well_conn_collection_iget_const( const well_conn_collection_type * wellcc , int index);
  well_conn_type       *      well_conn_collection_iget(const well_conn_collection_type * wellcc , int index);
  void                        well_conn_collection_add( well_conn_collection_type * wellcc , well_conn_type * conn);

#ifdef __cplusplus
}
#endif
#endif
