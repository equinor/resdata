/*
   Copyright (C) 2011  Statoil ASA, Norway. 
    
   The file 'ecl_grav_config.c' is part of ERT - Ensemble based
   Reservoir Tool.
    
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

#include <stdlib.h>
#include <math.h>
#include <stdbool.h>
#include <ecl_kw.h>
#include <ecl_util.h>
#include <ecl_file.h>
#include <ecl_grid.h>
#include <ecl_grav_config.h>
#include <util.h>



struct ecl_grav_config_struct {
  ecl_grid_type       * grid;
  ecl_file_type       * init_file;
  ecl_version_enum      ecl_version;
};



ecl_grav_config_type * ecl_grav_config_alloc( const char * grid_file , const char * init_file ) {
  ecl_grav_config_type * grav_config = util_malloc( sizeof * grav_config , __func__ );
  grav_config->grid        = ecl_grid_alloc( grid_file );
  grav_config->init_file   = ecl_file_fread_alloc( init_file );    
  grav_config->ecl_version = ecl_file_get_ecl_version( grav_config->init_file );
  return grav_config;
}



void ecl_grav_config_free( ecl_grav_config_type * grav_config ) {
  ecl_grid_free( grav_config->grid );
  ecl_file_free( grav_config->init_file );
  free( grav_config );
}
