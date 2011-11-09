/*
   Copyright (C) 2011  Statoil ASA, Norway. 
    
   The file 'geo_surface.c' is part of ERT - Ensemble based Reservoir Tool. 
    
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
#include <stdbool.h>
#include <util.h>
#include <geo_pointset.h>
#include <geo_surface.h>


/*

   3 ---- 2      
   |      |
   |      |
   0 ---- 1

*/


typedef struct {
  int index_list[4];
} cell_type;



struct geo_surface_struct {
  int         nx;
  int         ny;     
  
  cell_type * cells;
  geo_pointset_type * pointset;
};


static int geo_surface_cornerindex( const geo_surface_type * geo_surface , int cell_ix , int cell_iy) {
  return (geo_surface->nx + 1) * cell_iy + cell_ix;
}


static void geo_surface_init_cells( geo_surface_type * geo_surface ) {
  int ix,iy;
  geo_surface->cells = util_malloc( geo_surface->nx * geo_surface->ny * sizeof * geo_surface->cells , __func__);
  for (iy = 0; iy < geo_surface->ny; iy++) {
    for (ix = 0; ix < geo_surface->nx; ix++) {
      int cell_index = iy * geo_surface->nx + ix;
      
      geo_surface->cells[ cell_index ].index_list[0] = geo_surface_cornerindex( geo_surface , ix     , iy    );
      geo_surface->cells[ cell_index ].index_list[1] = geo_surface_cornerindex( geo_surface , ix + 1 , iy    );
      geo_surface->cells[ cell_index ].index_list[2] = geo_surface_cornerindex( geo_surface , ix + 1 , iy + 1);
      geo_surface->cells[ cell_index ].index_list[3] = geo_surface_cornerindex( geo_surface , ix     , iy + 1);
      
    }
  }
}


static void geo_surface_init_regular( geo_surface_type * surface , const double origo[2], const double vec1[2], const double vec2[2]) {
  int ix,iy;
  for (iy=0; iy <= surface->ny; iy++) {
    for (ix=0; ix <= surface->nx; ix++) {
      double x = origo[0] + ix*vec1[0] + iy*vec2[0];
      double y = origo[1] + ix*vec1[1] + iy*vec2[1];
      geo_pointset_add_xy( surface->pointset , x , y );
    }
  }
  geo_surface_init_cells( surface );
}


void geo_surface_free( geo_surface_type * surface ) {
  geo_pointset_free( surface->pointset );
  free( surface->cells );
  free( surface );
}










