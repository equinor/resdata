/*
   Copyright (C) 2013  Statoil ASA, Norway. 
    
   The file 'string_util.c' is part of ERT - Ensemble based Reservoir Tool. 
    
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

#include <ert/util/util.h>
#include <ert/util/int_vector.h>
#include <ert/util/bool_vector.h>
#include <ert/util/string_util.h>


static bool_vector_type * alloc_mask( const int_vector_type * active_list ) {
  bool_vector_type * mask = bool_vector_alloc( 0 , false );
  int i;
  for (i=0; i < int_vector_size( active_list ); i++) 
    bool_vector_iset( mask , int_vector_iget( active_list , i) , true );

  return mask;
}



void string_util_update_active_list( const char * range_string , int_vector_type * active_list ) {
  int_vector_sort( active_list );
  {
    bool_vector_type * mask = alloc_mask( active_list );
    string_util_update_active_mask( range_string , mask );

    int_vector_reset( active_list );
    {
      int i;
      for (i=0; i < bool_vector_size(mask); i++) {
        bool active = bool_vector_iget( mask , i );
        if (active)
          int_vector_append( active_list , i );
      }
    }
    
    bool_vector_free( mask );
  }
}


void string_util_init_active_list( const char * range_string , int_vector_type * active_list ) {
  int_vector_reset( active_list );
  string_util_update_active_list( range_string , active_list );
}


int_vector_type *  string_util_alloc_active_list( const char * range_string ) {
  int_vector_type * active_list = int_vector_alloc( 0 , 0 );
  string_util_init_active_list( range_string , active_list );
  return active_list;
}

/*****************************************************************/

/*
  This is the only function which actually invokes the low level
  string parsing in util_sscanf_alloc_active_list().  
*/

void string_util_update_active_mask( const char * range_string , bool_vector_type * active_mask) {
  int length , i;
  int * sscanf_active = util_sscanf_alloc_active_list( range_string , &length);
  for (i=0; i < length; i++)
    bool_vector_iset( active_mask , sscanf_active[i] , true );
  
  util_safe_free( sscanf_active );
}


void string_util_init_active_mask( const char * range_string , bool_vector_type * active_mask ) {
  bool_vector_reset( active_mask );
  string_util_update_active_mask( range_string , active_mask );
}


bool_vector_type * string_util_alloc_active_mask( const char * range_string ) {
  bool_vector_type * mask  = bool_vector_alloc(0 , false );
  string_util_init_active_mask( range_string , mask );
  return mask;
}
