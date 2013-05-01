/*
   Copyright (C) 2011  Statoil ASA, Norway. 
   
   The file 'well_conn.c' is part of ERT - Ensemble based Reservoir Tool. 
    
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

#include <ert/ecl/ecl_kw.h>
#include <ert/ecl/ecl_rsthead.h>

#include <ert/ecl_well/well_const.h>
#include <ert/ecl_well/well_conn.h>


#define WELL_CONN_NORMAL_WELL_SEGMENT_ID -999
#define ECLIPSE_NORMAL_WELL_SEGMENT_ID     -1

/*
  Observe that when the (ijk) values are initialized they are
  shifted to zero offset values, to be aligned with the rest of the
  ert libraries. 
*/

struct  well_conn_struct {
    int                i;
    int                j;
    int                k;
    well_conn_dir_enum dir;
    bool               open;         
    int                segment;             // -1: Ordinary well
    bool               matrix_connection;   // k >= nz => fracture (and k -= nz )
};
  


static void well_conn_set_k( well_conn_type * conn , const ecl_rsthead_type * header , int icon_k) {
  conn->k = icon_k;
  conn->matrix_connection = true;

  if (header->dualp) {
    int geometric_nz = header->nz / 2;
    if (icon_k >= geometric_nz) {
      conn->k -= geometric_nz;
      conn->matrix_connection = false;
    }
  }
}




well_conn_type * well_conn_alloc_wellhead( const ecl_kw_type * iwel_kw , const ecl_rsthead_type * header , int well_nr)  {
  const int iwel_offset = header->niwelz * well_nr;
  int conn_i = ecl_kw_iget_int( iwel_kw , iwel_offset + IWEL_HEADI_ITEM );
  
  if (conn_i > 0) {
    well_conn_type * conn = util_malloc( sizeof * conn );
    
    conn->i    = ecl_kw_iget_int( iwel_kw , iwel_offset + IWEL_HEADI_ITEM ) - 1;
    conn->j    = ecl_kw_iget_int( iwel_kw , iwel_offset + IWEL_HEADJ_ITEM ) - 1;
    {
      int icon_k = ecl_kw_iget_int( iwel_kw , iwel_offset + IWEL_HEADK_ITEM ) - 1;
      well_conn_set_k( conn , header , icon_k);
    }
   
    conn->open           = true;  // This is not really specified anywhere.
    conn->segment        = 0;   
    /*
      conn->branch         = 0;
      conn->outlet_segment = 0;
    */
    return conn;
  } else
    // The well is completed in this LGR - however the wellhead is in another LGR.
    return NULL;
}


bool well_conn_MSW( const well_conn_type * conn ) {
  if (conn->segment == WELL_CONN_NORMAL_WELL_SEGMENT_ID)
    return false;
  else
    return true;
}

static bool well_conn_assert_direction( well_conn_dir_enum dir, bool matrix_connection) {
  if ((dir == well_conn_fracX || dir == well_conn_fracY) && matrix_connection)
    return false;
  else
    return true;
}




static well_conn_type * well_conn_alloc__( int i , int j , int k , well_conn_dir_enum dir , bool open, int segment_id, bool matrix_connection) {
  if (well_conn_assert_direction( dir , matrix_connection)) {
    well_conn_type * conn = util_malloc( sizeof * conn );
    conn->i = i;
    conn->j = j;
    conn->k = k;
    conn->open = open;
    conn->dir = dir;
    
    conn->matrix_connection = matrix_connection;
    if (segment_id == ECLIPSE_NORMAL_WELL_SEGMENT_ID)
      conn->segment = WELL_CONN_NORMAL_WELL_SEGMENT_ID;
    else
      conn->segment = segment_id;

    return conn;
  } else {
    printf("assert-direction failed.  dir:%d  matrix_connection:%d \n",dir , matrix_connection);
    return NULL;
  }
}


well_conn_type * well_conn_alloc( int i , int j , int k , well_conn_dir_enum dir , bool open) {
  return well_conn_alloc__(i , j , k , dir , open , WELL_CONN_NORMAL_WELL_SEGMENT_ID , true );
}



well_conn_type * well_conn_alloc_MSW( int i , int j , int k , well_conn_dir_enum dir , bool open, int segment) {
  return well_conn_alloc__(i , j , k , dir , open , segment , true );
}



well_conn_type * well_conn_alloc_fracture( int i , int j , int k , well_conn_dir_enum dir , bool open) {
  return well_conn_alloc__(i , j , k , dir , open , WELL_CONN_NORMAL_WELL_SEGMENT_ID , false);
}



well_conn_type * well_conn_alloc_fracture_MSW( int i , int j , int k , well_conn_dir_enum dir , bool open, int segment) {
  return well_conn_alloc__(i , j , k , dir , open , segment , false);
}




/*
  Observe that the (ijk) and branch values are shifted to zero offset to be
  aligned with the rest of the ert libraries.  
*/
well_conn_type * well_conn_alloc_from_kw( const ecl_kw_type * icon_kw , 
                                          const ecl_rsthead_type * header , 
                                          int well_nr , 
                                          int conn_nr ) {
  
  const int icon_offset = header->niconz * ( header->ncwmax * well_nr + conn_nr );
  int IC = ecl_kw_iget_int( icon_kw , icon_offset + ICON_IC_ITEM );
  if (IC > 0) {
    well_conn_type * conn;
    int i       = ecl_kw_iget_int( icon_kw , icon_offset + ICON_I_ITEM ) - 1;
    int j       = ecl_kw_iget_int( icon_kw , icon_offset + ICON_J_ITEM ) - 1;
    int k       = ecl_kw_iget_int( icon_kw , icon_offset + ICON_K_ITEM ) - 1;
    int segment = ecl_kw_iget_int( icon_kw , icon_offset + ICON_SEGMENT_ITEM ) - 1;
    bool matrix_connection = true;
    bool open;
    well_conn_dir_enum dir = well_conn_fracX;
    
    /* Set the status */
    {
      int int_status = ecl_kw_iget_int( icon_kw , icon_offset + ICON_STATUS_ITEM );
      if (int_status > 0)
        open = true;
      else
        open = false;
    }
    
    
    /* Set the K value and fracture flag. */
    {
      if (header->dualp) {
        int geometric_nz = header->nz / 2;
        if (k >= geometric_nz) {
          k -= geometric_nz;
          matrix_connection = false;
        }
      }
    }
    
    
    /* Set the direction flag */
    {
      int int_direction = ecl_kw_iget_int( icon_kw , icon_offset + ICON_DIRECTION_ITEM );
      if (int_direction == ICON_DEFAULT_DIR_VALUE)
        int_direction = ICON_DEFAULT_DIR_TARGET;
      
      switch (int_direction) {
      case(ICON_DIRX):
        dir = well_conn_dirX;
        break;
      case(ICON_DIRY):
        dir = well_conn_dirY;
        break;
      case(ICON_DIRZ):
        dir = well_conn_dirZ;
        break;
      case(ICON_FRACX):
        dir = well_conn_fracX;
        break;
      case(ICON_FRACY):
        dir = well_conn_fracY;
        break;
      default:
        util_abort("%s: icon direction value:%d not recognized\n",__func__ , int_direction);
      }
    }
    
    conn = well_conn_alloc__(i,j,k,dir,open,segment,matrix_connection);
    
    /**
       For multisegmented wells ONLY the global part of the restart
       file has segment information, i.e. the ?SEG
       keywords. Consequently iseg_kw will be NULL for the part of a
       MSW + LGR well.
    */
    
    /*
    if (iseg_kw != NULL) {
      if (conn->segment != WELL_CONN_NORMAL_WELL_SEGMENT_ID) {
        const int iseg_offset = header->nisegz * ( header->nsegmx * seg_well_nr + conn->segment );
        conn->outlet_segment = ecl_kw_iget_int( iseg_kw , iseg_offset + ISEG_OUTLET_ITEM );  
        conn->branch         = ecl_kw_iget_int( iseg_kw , iseg_offset + ISEG_BRANCH_ITEM );  
      } else {
        conn->branch = 0;
        conn->outlet_segment = 0;
      }
    } else {
      conn->branch = 0;
      conn->outlet_segment = 0;
    }
    */
    
    return conn;
  } else
    return NULL;  /* IC < 0: Connection not in current LGR. */
}


void well_conn_free( well_conn_type * conn) {
  free( conn );
}


void well_conn_free__( void * arg ) {
  well_conn_type * conn = (well_conn_type *) arg;
  well_conn_free( conn );
}



/*****************************************************************/

int well_conn_get_i(const well_conn_type * conn) {
  return conn->i;
}

int well_conn_get_j(const well_conn_type * conn) {
  return conn->j;
}

int well_conn_get_k(const well_conn_type * conn) {
  return conn->k;
}



well_conn_dir_enum well_conn_get_dir(const well_conn_type * conn) {
  return conn->dir;
}

bool well_conn_open( const well_conn_type * conn ) {
  return conn->open;
}


int well_conn_get_segment( const well_conn_type * conn ) {
  return conn->segment;
}

bool well_conn_fracture_connection( const well_conn_type * conn) {
  return !conn->matrix_connection;
}

bool well_conn_matrix_connection( const well_conn_type * conn) {
  return conn->matrix_connection;
}

