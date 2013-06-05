/*
   Copyright (C) 2011  Statoil ASA, Norway. 
    
   The file 'ecl_rft_node.c' is part of ERT - Ensemble based Reservoir Tool. 
    
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

#include <math.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>

#include <ert/util/util.h>
#include <ert/util/hash.h>

#include <ert/ecl/ecl_kw.h>
#include <ert/ecl/ecl_kw_magic.h>
#include <ert/ecl/ecl_file.h>
#include <ert/ecl/ecl_rft_node.h>


/** 
    The RFT's from several wells, and possibly also several timesteps
    are lumped togeheter in one .RFT file. The ecl_rft_node
    implemented in this file contains the information for one
    well/report step. 
*/
    


/*
  If the type is RFT, PLT or SEGMENT depends on the options used when
  the .RFT file is created. RFT and PLT are quite similar, SEGMENT is
  not really supported.
*/


/** 
  Here comes some small structs containing various pieces of
  information. Observe the following which is common to all these
  structs:

   * In the implementation only 'full instance' are employed, and
     not pointers. This implies that the code does not provide
     xxx_alloc() and xx_free() functions.

   * They should NOT be exported out of this file.
*/


/** 
    This type is used to hold the coordinates of a perforated
    cell. This type is used irrespective of whether this is a simple
    RFT or a PLT.
*/

typedef struct {
  int    i;
  int    j;
  int    k; 
  double depth;
  double pressure; /* both CONPRES from PLT and PRESSURE from RFT are internalized as pressure in the cell_type. */
} cell_type;

/*-----------------------------------------------------------------*/
/** 
    Type which contains the information for one cell in an RFT.
*/
typedef struct {
  double swat;
  double sgas;
} rft_data_type;


/*-----------------------------------------------------------------*/
/** 
    Type which contains the information for one cell in an PLT.
*/
typedef struct {
  double orat;
  double grat;
  double wrat;
  /* There is quite a lot of more information in the PLT - not yet internalized. */
} plt_data_type;




/* This is not implemented at all ..... */
typedef struct {
  double data;
} segment_data_type;


#define ECL_RFT_NODE_ID 887195
struct ecl_rft_node_struct {
  UTIL_TYPE_ID_DECLARATION;
  char       * well_name;              /* Name of the well. */
  int          size;                   /* The number of entries in this RFT vector (i.e. the number of cells) .*/

  ecl_rft_enum data_type;              /* What type of data: RFT|PLT|SEGMENT */
  time_t       recording_date;         /* When was the RFT recorded - date.*/ 
  double       days;                   /* When was the RFT recorded - days after simulaton start. */
  cell_type    *cells;                 /* Coordinates and depth of the well cells. */

  /* Only one of segment_data, rft_data or plt_data can be != NULL */
  segment_data_type * segment_data;    
  rft_data_type     * rft_data;
  plt_data_type     * plt_data;
};



/**
   Will return NULL if the data_type_string is equal to "SEGMENT" -
   that is not (yet) supported.
*/

static ecl_rft_node_type * ecl_rft_node_alloc_empty(int size , const char * data_type_string) {
  ecl_rft_enum data_type = SEGMENT;
  
  /* According to the ECLIPSE documentaton. */
  if (strchr(data_type_string , 'P') != NULL)
    data_type = PLT;
  else if (strchr(data_type_string, 'R') != NULL)
    data_type = RFT;
  else if (strchr(data_type_string , 'S') != NULL)
    data_type = SEGMENT;
  else 
    util_abort("%s: Could not determine type of RFT/PLT/SEGMENT data - aborting\n",__func__);
  

  /* Can return NULL */
  if (data_type == SEGMENT) {
    fprintf(stderr,"%s: sorry SEGMENT PLT/RFT is not supported - file a complaint. \n",__func__);
    return NULL;
  }

  {
    ecl_rft_node_type * rft_node = util_malloc(sizeof * rft_node );
    rft_node->plt_data     = NULL;
    rft_node->rft_data     = NULL;
    rft_node->segment_data = NULL;
    
    UTIL_TYPE_ID_INIT( rft_node , ECL_RFT_NODE_ID );
    
    rft_node->cells = util_calloc( size , sizeof * rft_node->cells );
    if (data_type == RFT)
      rft_node->rft_data = util_calloc( size , sizeof * rft_node->rft_data );
    else if (data_type == PLT)
      rft_node->plt_data = util_calloc( size , sizeof * rft_node->plt_data);
    else if (data_type == SEGMENT)
      rft_node->segment_data = util_calloc( size , sizeof * rft_node->segment_data );
    
    rft_node->size = size;
    rft_node->data_type = data_type;

    return rft_node;
  }
}


UTIL_SAFE_CAST_FUNCTION( ecl_rft_node   , ECL_RFT_NODE_ID );
UTIL_IS_INSTANCE_FUNCTION( ecl_rft_node , ECL_RFT_NODE_ID );


ecl_rft_node_type * ecl_rft_node_alloc(const ecl_file_type * rft) {
  ecl_kw_type       * conipos   = ecl_file_iget_named_kw(rft , CONIPOS_KW , 0);
  ecl_kw_type       * welletc   = ecl_file_iget_named_kw(rft , WELLETC_KW , 0);
  ecl_rft_node_type * rft_node  = ecl_rft_node_alloc_empty(ecl_kw_get_size(conipos) , 
                                                           ecl_kw_iget_ptr(welletc , WELLETC_TYPE_INDEX));
  
  if (rft_node != NULL) {
    ecl_kw_type * date_kw = ecl_file_iget_named_kw( rft , DATE_KW    , 0);
    ecl_kw_type * conjpos = ecl_file_iget_named_kw( rft , CONJPOS_KW , 0);
    ecl_kw_type * conkpos = ecl_file_iget_named_kw( rft , CONKPOS_KW , 0);
    ecl_kw_type * depth_kw;
    if (rft_node->data_type == RFT)
      depth_kw = ecl_file_iget_named_kw( rft , DEPTH_KW , 0);
    else
      depth_kw = ecl_file_iget_named_kw( rft , CONDEPTH_KW , 0);
    rft_node->well_name = util_alloc_strip_copy( ecl_kw_iget_ptr(welletc , WELLETC_NAME_INDEX));

    /* Time information. */
    {
      int * time = ecl_kw_get_int_ptr( date_kw );
      rft_node->recording_date = util_make_date( time[DATE_DAY_INDEX] , time[DATE_MONTH_INDEX] , time[DATE_YEAR_INDEX] );
    }
    rft_node->days = ecl_kw_iget_float( ecl_file_iget_named_kw( rft , TIME_KW , 0 ) , 0);
    
    
    /* Cell information */
    {
      const int   * i      = ecl_kw_get_int_ptr( conipos );
      const int   * j      = ecl_kw_get_int_ptr( conjpos );
      const int   * k      = ecl_kw_get_int_ptr( conkpos );
      const float * depth  = ecl_kw_get_float_ptr( depth_kw );


      /* The ECLIPSE file has offset-1 coordinates, and that is
         currently what we store; What a fxxxing mess. */
      int c;
      for (c = 0; c < rft_node->size; c++) {
        rft_node->cells[c].i     = i[c];
        rft_node->cells[c].j     = j[c];
        rft_node->cells[c].k     = k[c];
        rft_node->cells[c].depth = depth[c];
      }
        
    }

    
    /* Now we are done with the information which is common to both RFT and PLT. */
    if (rft_node->data_type == RFT) {
      const float * SW = ecl_kw_get_float_ptr( ecl_file_iget_named_kw( rft , SWAT_KW , 0));
      const float * SG = ecl_kw_get_float_ptr( ecl_file_iget_named_kw( rft , SGAS_KW , 0)); 
      const float * P  = ecl_kw_get_float_ptr( ecl_file_iget_named_kw( rft , PRESSURE_KW , 0));
          int c;
      for (c = 0; c < rft_node->size; c++) {
        rft_node->rft_data[c].swat     = SW[c];
        rft_node->rft_data[c].sgas     = SG[c];
        rft_node->cells[c].pressure     = P[c];
      }
    } else if (rft_node->data_type == PLT) {
      /* For PLT there is quite a lot of extra information which is not yet internalized. */
      const float * WR = ecl_kw_get_float_ptr( ecl_file_iget_named_kw( rft , CONWRAT_KW , 0));
      const float * GR = ecl_kw_get_float_ptr( ecl_file_iget_named_kw( rft , CONGRAT_KW , 0)); 
      const float * OR = ecl_kw_get_float_ptr( ecl_file_iget_named_kw( rft , CONORAT_KW , 0)); 
      const float * P  = ecl_kw_get_float_ptr( ecl_file_iget_named_kw( rft , CONPRES_KW , 0));
          int c;
      for ( c = 0; c < rft_node->size; c++) {
        rft_node->plt_data[c].orat     = OR[c];
        rft_node->plt_data[c].grat     = GR[c];
        rft_node->plt_data[c].wrat     = WR[c];
        rft_node->cells[c].pressure     = P[c];   
      }
    } else {
      /* Segment code - not implemented. */
    }
  }
  return rft_node;
}


const char * ecl_rft_node_get_well_name(const ecl_rft_node_type * rft_node) { 
  return rft_node->well_name; 
}


void ecl_rft_node_free(ecl_rft_node_type * rft_node) {
  free(rft_node->well_name);
  free(rft_node->cells);
  util_safe_free(rft_node->segment_data);
  util_safe_free(rft_node->rft_data);
  util_safe_free(rft_node->plt_data);
  free(rft_node);
}

void ecl_rft_node_free__(void * void_node) {
  ecl_rft_node_free( ecl_rft_node_safe_cast (void_node) );
}



void ecl_rft_node_summarize(const ecl_rft_node_type * rft_node , bool print_cells) {
  int i;
  printf("--------------------------------------------------------------\n");
  printf("Well.............: %s \n",rft_node->well_name);
  printf("Completed cells..: %d \n",rft_node->size);
  {
    int day , month , year;
    util_set_date_values(rft_node->recording_date , &day , &month , &year);
    printf("Recording date...: %02d/%02d/%4d / %g days after simulation start.\n" , day , month , year , rft_node->days);
  }
  printf("-----------------------------------------\n");
  if (print_cells) {
    printf("     i   j   k       Depth       Pressure\n");
    printf("-----------------------------------------\n");
    for (i=0; i < rft_node->size; i++) 
      printf("%2d: %3d %3d %3d | %10.2f %10.2f \n",i,
             rft_node->cells[i].i , 
             rft_node->cells[i].j , 
             rft_node->cells[i].k , 
             rft_node->cells[i].depth,
             rft_node->cells[i].pressure);
    printf("-----------------------------------------\n");
  }
}



int          ecl_rft_node_get_size(const ecl_rft_node_type * rft_node) { return rft_node->size; }
time_t       ecl_rft_node_get_date(const ecl_rft_node_type * rft_node) { return rft_node->recording_date; }
ecl_rft_enum ecl_rft_node_get_type(const ecl_rft_node_type * rft_node) { return rft_node->data_type; }


/*****************************************************************/
/* various functions to access properties at the cell level      */

static cell_type * ecl_rft_node_iget_cell( const ecl_rft_node_type * rft_node , int index) {
  if (index < rft_node->size)
    return &rft_node->cells[index];
  else {
    util_abort("%s: asked for cell:%d max:%d \n",__func__ , index , rft_node->size - 1);
    return NULL;
  }
}



double ecl_rft_node_iget_depth( const ecl_rft_node_type * rft_node , int index) {
  const cell_type * cell = ecl_rft_node_iget_cell( rft_node , index );
  return cell->depth;
}



double ecl_rft_node_iget_pressure( const ecl_rft_node_type * rft_node , int index) {
  const cell_type * cell = ecl_rft_node_iget_cell( rft_node , index );
  return cell->pressure;
}


void ecl_rft_node_iget_ijk( const ecl_rft_node_type * rft_node , int index , int *i , int *j , int *k) {
  const cell_type * cell = ecl_rft_node_iget_cell( rft_node , index );
  *i = cell->i;
  *j = cell->j;
  *k = cell->k;
}


int ecl_rft_node_lookup_ijk( const ecl_rft_node_type * rft_node , int i, int j , int k) {
  int index = 0; 
  while (true) {
    const cell_type * cell = ecl_rft_node_iget_cell( rft_node , index );
    if ((cell->i == (i+1)) && (cell->j == (j+1)) && (cell->k == (k+1)))  /* OK - found it. */
      return index;
    
    index++;
    if (index == rft_node->size)                             /* Could not find it. */
      return -1;
  }
}


static void assert_type_and_index( const ecl_rft_node_type * rft_node , ecl_rft_enum target_type , int index) {
  if (rft_node->data_type != target_type)
    util_abort("%s: wrong type \n",__func__);

  if ((index < 0) || (index >= rft_node->size))
    util_abort("%s: invalid index:%d \n",__func__ , index);
}

double ecl_rft_node_iget_sgas( const ecl_rft_node_type * rft_node , int index) {
  assert_type_and_index( rft_node , RFT , index );
  {
    const rft_data_type rft_data = rft_node->rft_data[ index ];
    return rft_data.sgas;
  }
}


double ecl_rft_node_iget_swat( const ecl_rft_node_type * rft_node , int index) {
  assert_type_and_index( rft_node , RFT , index );
  {
    const rft_data_type rft_data = rft_node->rft_data[ index ];
    return rft_data.swat;
  }
}


double ecl_rft_node_iget_orat( const ecl_rft_node_type * rft_node , int index) {
  assert_type_and_index( rft_node , PLT , index );
  {
    const plt_data_type  plt_data = rft_node->plt_data[ index ];
    return plt_data.orat;
  }
}


double ecl_rft_node_iget_wrat( const ecl_rft_node_type * rft_node , int index) {
  assert_type_and_index( rft_node , PLT , index );
  {
    const plt_data_type  plt_data = rft_node->plt_data[ index ];
    return plt_data.wrat;
  }
}


double ecl_rft_node_iget_grat( const ecl_rft_node_type * rft_node , int index) {
  assert_type_and_index( rft_node , PLT , index );
  {
    const plt_data_type  plt_data = rft_node->plt_data[ index ];
    return plt_data.grat;
  }
}



bool ecl_rft_node_is_PLT( const ecl_rft_node_type * rft_node ) {
  if (rft_node->data_type == PLT)
    return true;
  else
    return false;
}



bool ecl_rft_node_is_RFT( const ecl_rft_node_type * rft_node ) {
  if (rft_node->data_type == RFT)
    return true;
  else
    return false;
}


