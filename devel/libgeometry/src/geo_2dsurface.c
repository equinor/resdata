/*
   Copyright (C) 2011  Statoil ASA, Norway. 
    
   The file 'surface.c' is part of ERT - Ensemble based Reservoir Tool. 
    
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
#include <surface.h>



typedef struct {
  int index_list[4];
} cell_type;



struct surface_struct {
  int         nx;
  int         ny;     
  
  cell_type * cells;
  double    * xcoord;
  double    * ycoord;
  double    * zcoord;     // Can be NULL 
};


static int surface_cornerindex( const surface_type * surface , int cell_ix , int cell_iy) {
  return (surface->nx + 2) * cell_iy + cell_ix;
}


static void surface_init_cells( surface_type * surface ) {
  int ix,iy;
  surface->cells = util_malloc( surface->nx * surface->ny * sizeof * surface->cells , __func__);
  for (iy = 0; iy < surface->ny; iy++) {
    for (ix=0; ix < surface->nx; ix++) {
      int global_index = iy * surface->nx + ix;
      
      surface->cells[ global_index ].index_list[0] = surface_cornerindex( surface , ix     , iy    );
      surface->cells[ global_index ].index_list[1] = surface_cornerindex( surface , ix + 1 , iy    );
      surface->cells[ global_index ].index_list[2] = surface_cornerindex( surface , ix + 1 , iy + 1);
      surface->cells[ global_index ].index_list[3] = surface_cornerindex( surface , ix     , iy + 1);
      
    }
  }
}


void surface_free( surface_type * surface ) {
  free( surface->cells );
  free( surface->xcoord );
  free( surface->ycoord );
  util_safe_free( surface->zcoord );
}








