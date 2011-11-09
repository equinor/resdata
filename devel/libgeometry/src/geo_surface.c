/*
   Copyright (C) 2011  Statoil ASA, Norway. 
    
   The file 'geo_2dsurface.c' is part of ERT - Ensemble based Reservoir Tool. 
    
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
#include <geo_2dsurface.h>



/*

   3 ---- 2      
   |      |
   |      |
   0 ---- 1

*/


typedef struct {
  int index_list[4];
} cell_type;



struct geo_2dsurface_struct {
  int         nx;
  int         ny;     
  
  cell_type * cells;
  geo_pointset_type * pointset;
};


static int geo_2dsurface_cornerindex( const geo_2dsurface_type * geo_2dsurface , int cell_ix , int cell_iy) {
  return (geo_2dsurface->nx + 1) * cell_iy + cell_ix;
}


static void geo_2dsurface_init_cells( geo_2dsurface_type * geo_2dsurface ) {
  int ix,iy;
  geo_2dsurface->cells = util_malloc( geo_2dsurface->nx * geo_2dsurface->ny * sizeof * geo_2dsurface->cells , __func__);
  for (iy = 0; iy < geo_2dsurface->ny; iy++) {
    for (ix = 0; ix < geo_2dsurface->nx; ix++) {
      int cell_index = iy * geo_2dsurface->nx + ix;
      
      geo_2dsurface->cells[ cell_index ].index_list[0] = geo_2dsurface_cornerindex( geo_2dsurface , ix     , iy    );
      geo_2dsurface->cells[ cell_index ].index_list[1] = geo_2dsurface_cornerindex( geo_2dsurface , ix + 1 , iy    );
      geo_2dsurface->cells[ cell_index ].index_list[2] = geo_2dsurface_cornerindex( geo_2dsurface , ix + 1 , iy + 1);
      geo_2dsurface->cells[ cell_index ].index_list[3] = geo_2dsurface_cornerindex( geo_2dsurface , ix     , iy + 1);
      
    }
  }
}


static void geo_2dsurface_init_regular( geo_2dsurface_type * surface , const double origo[2], const double vec1[2], const double vec2[2]) {
  int ix,iy;
  for (iy=0; iy <= surface->ny; iy++) {
    for (ix=0; ix <= surface->nx; ix++) {
      double x = origo[0] + ix*vec1[0] + iy*vec2[0];
      double y = origo[1] + ix*vec1[1] + iy*vec2[1];
      geo_pointset_add_xy( surface->pointset , x , y );
    }
  }
  geo_2dsurface_init_cells( surface );
}


void geo_2dsurface_free( geo_2dsurface_type * surface ) {
  geo_pointset_free( surface->pointset );
  free( surface->cells );
  free( surface );
}










