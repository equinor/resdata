/*
   Copyright (C) 2011  Statoil ASA, Norway. 
    
   The file 'ecl_grav_config.h' is part of ERT - Ensemble based
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

#ifndef __ECL_GRAV_CONFIG_H__
#define __ECL_GRAV_CONFIG_H__
#ifdef __plusplus
extern "C" {
#endif


typedef struct ecl_grav_config_struct ecl_grav_config_type;

void                   ecl_grav_config_free( ecl_grav_config_type * ecl_grav_config );
ecl_grav_config_type * ecl_grav_config_alloc( const char * grid_file , const char * init_file );

#ifdef __plusplus
}
#endif
#endif
