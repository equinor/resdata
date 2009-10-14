#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <util.h>
#include <ecl_kw.h>
#include <ecl_grid.h>
#include <stdbool.h>
#include <ecl_util.h>
#include <double_vector.h>
#include <int_vector.h>
#include <ecl_file.h>
#include <hash.h>
#include <vector.h>
#include <stringlist.h>
#include <point.h>


/**
  This function implements functionality to load ECLISPE grid files,
  both .EGRID and .GRID files - in a transparent fashion.

  Observe the following convention:

  global_index:  [0 , nx*ny*nz)
  active_index:  [0 , nactive)

  About indexing
  --------------

  There are three different ways to index/access a cell:

    1. By ijk
    2. By global index, [0 , nx*ny*nz)
    3. By active index, [0 , nactive)

  Most of the query functions can take input in several of the
  ways. The expected arguments are indicated as the last part of the
  function name:

    ecl_grid_get_pos3()  - 3:  this function expects i,j,k
    ecl_grid_get_pos1()  - 1:  this function expects a global index
    ecl_grid_get_pos1A() - 1A: this function expects an active index.
    
*/


/**
   Note about LGR
   --------------

   The ECLIPSE Local Grid Refinement (LGR) is organised as follows:

     1. You start with a normal grid.
     2. Some of the cells can be subdivided into further internal
        grids, this is the LGR.

   This is illustrated below:


    +--------------------------------------+
    |  	       	 |	      |		   |
    |		 |	      |		   |
    |	  X	 | 	X     |	    X 	   |
    |		 |	      |		   |
    |		 |	      |		   |
    -------------|------------|------------|
    |  	  | 	 |      |     |		   |
    |     |  x 	 |   x 	|     |		   |
    |-----X------|------X-----|	    X	   |
    |  x  |  x   |   x 	|     |	     	   |
    |  	  |	 |	|     |	     	   |
    -------------|------------|------------|
    |	   	 |	      |	     	   |
    |	   	 |	      |	     	   |
    |	  X	 |     	      |	     	   |
    |		 | 	      |	     	   |
    |		 | 	      |		   |
    +--------------------------------------+


  The grid above shows the following:

    1. The coarse (i.e. normal) grid has 9 cells, of which 7 marked
       with 'X' are active.

    2. Two of the cells have been refined into new 2x2 grids. In the
       refined cells only three and two of the refined cells are
       active.

  In a GRID file the keywords for this grid will look like like this:


  .....    __
  COORDS     \
  CORNERS     |
  COORDS      |
  CORNERS     |
  COORDS      |
  CORNERS     |      Normal COORD / CORNERS kewyords
  COORDS      |      for the nine coarse cells. Observe
  CORNERS     |      that when reading in these cells it is
  COORDS      |      IMPOSSIBLE to know that some of the
  CORNERS     |      cells will be subdivieded in a following
  COORDS      |      LGR definition.
  CORNERS     |
  COORDS      |
  CORNERS     |
  COORDS      |    
  CORNERS     |      
  COORDS      |      
  CORNERS  __/________________________________________________________  
  LGR        \
  LGRILG      |     
  DIMENS      |       
  COORDS      |
  CORNERS     |      First LGR, with some header information, 
  COORDS      |      and then normal COORDS/CORNERS keywords for
  CORNERS     |      the four refined cells.
  COORDS      |
  CORNERS     |
  COORDS      |
  CORNERS  __/
  LGR	     \
  LGRILG      |
  DIMENS      |
  COORDS      |      Second LGR.
  CORNERS     |
  COORDS      |
  CORNERS     |
  COORDS      |
  CORNERS     |
  COORDS      |
  CORNERS  __/

  
  For EGRID files it is essentially the same, except for replacing the
  keywords COORDS/CORNERS with COORD/ZCORN/ACTNUM. Also the LGR
  headers are somewhat different.  

  Solution data in restart files comes in a similar way, a restart
  file with LGR can typically look like this:

  .....   __
  .....     \ 
  STARTSOL   |   All restart data for the ordinary
  PRESSURE   |   grid.
  SWAT       |
  SGAS       |
  ....       |
  ENDSOL  __/____________________________ 
  LGR       \
  ....       |
  STARTSOL   |   Restart data for 
  PRESSURE   |   the first LGR.
  SGAS       |
  SWAT       |
  ...        |
  ENDSOL     |
  ENDLGR  __/   
  LGR       \ 
  ....       |
  STARTSOL   |   Restart data for 
  PRESSURE   |   the second LGR.
  SGAS       |
  SWAT       |
  ...        |
  ENDSOL     |
  ENDLGR  __/


  The LGR implementation in is based on the following main principles:

   1. When loading a EGRID/GRID file one ecl_grid_type instance will
      be allocated; this grid will contain the main grid, and all the
      lgr grids. 

   2. Only one datatype (ecl_grid_type) is used both for the main grid
      and the lgr grids.

   3. The main grid will own (memory wise) all the lgr grids, this
      even applies to nested subgrids whose parent is also a lgr.

   4. When it comes to indexing and so on there is no difference
      between lgr grid and the main grid.


      Example:
      --------

      {
      	 ecl_file_type * restart_data = ecl_file_fread_alloc(restart_filename , true); 		            // load some restart info to inspect
      	 ecl_grid_type * grid         = ecl_grid_alloc(grid_filename , true);          		            // bootstrap ecl_grid instance
      	 stringlist_type * lgr_names  = ecl_grid_alloc_name_list( grid );                                   // get a list of all the LGR names.
      
      	 printf("Grid:%s has %d a total of %d LGR's \n", grid_filename , stringlist_get_size( lgr_names ));
      	 for (int lgr_nr = 0; lgr_nr < stringlist_get_size( lgr_names); lgr_nr++) {
      	    ecl_grid_type * lgr_grid  = ecl_grid_get_lgr( grid , stringlist_iget( lgr_names , lgr_nr ));    // Get the ecl_grid instance of the lgr - by name.
      	    ecl_kw_type   * pressure_kw;
	    int nx,ny,nz,active_size;
	    ecl_grid_get_dims( lgr_grid , &nx , &ny , &nz , &active_size);                             // Get some size info from this lgr.
	    printf("LGR:%s has %d x %d x %d elements \n",stringlist_iget(lgr_names , lgr_nr ) , nx , ny , nz);

	    // OK - now we want to extract the solution vector (pressure) corresponding to this lgr:
	    pressure_kw = ecl_file_iget_named_kw( ecl_file , "PRESSURE" , ecl_grid_get_grid_nr( lgr_grid ));
      	                                                                              /|\
      	                                                                               |
      	                                                                               |   
      	                                                                We query the lgr_grid instance to find which
      	 								occurence of the solution data we should look
									up in the ecl_file instance with restart data. Puuhh!!

      	    {
      	       int center_index = ecl_grid_get_global_index3( lgr_grid , nx/2 , ny/2 , nz/2 );          // Ask the lgr_grid to get the index at the center of the lgr grid.
      	       printf("The pressure in the middle of %s is %g \n", stinglist_iget( lgr_names , lgr_nr ) , ecl_kw_iget_as_double( pressure_kw , center_index ));
            }
      	 }
      	 ecl_file_free( restart_data );
      	 ecl_grid_free( grid );
      	 stringlist_free( lgr_names );
     }

*/



/*

  The implementation is based on a hierarchy of three datatypes:

   1. ecl_grid   - This is the only exported datatype
   2. ecl_cell   - Internal
   3. point      - Implemented in file point.c

*/


typedef struct ecl_cell_struct  ecl_cell_type;


struct ecl_cell_struct {
  bool 		         active;
  int  		         active_index;
  point_type            *center;
  point_type            *corner_list[8];
  const ecl_grid_type   *lgr;            /* If this cell is part of an LGR; this will point to a grid instance for that LGR; NULL if not part of LGR. */
  int                    host_cell;      /* The global index of the host cell for an LGR cell, set to -1 for normal cells. */
};  



#define ECL_GRID_ID 991010
struct ecl_grid_struct {
  UTIL_TYPE_ID_DECLARATION;
  int                   grid_nr;       /* This corresponds to item 4 in GRIDHEAD - 0 for the main grid. */ 
  char                * name;          /* The name of the file for the main grid - name of the LGR for LGRs. */
  int                   ny,nz,nx;
  int                   size;          /* == nx*ny*nz */
  int                   total_active; 
  int                 * index_map;     /* This a list of nx*ny*nz elements, where value -1 means inactive cell .*/
  int                 * inv_index_map; /* This is list of total_active elements - which point back to the index_map. */
  ecl_cell_type      ** cells;         

  char                * parent_name;   /* The name of the parent for a nested LGR - for a LGR descending directly from the main grid this will be NULL. */
  hash_type           * children;      /* A table of LGR children for this grid. */
  const ecl_grid_type * parent_grid;   /* The parent grid for this (lgr) - NULL for the main grid. */      
  
  /*
    The two fields below are for *storing* LGR grid instances. Observe
    that these fields will be NULL for LGR grids, i.e. grids with
    grid_nr > 0. 
  */
  vector_type         * LGR_list;      /* An vector of ecl_grid instances for LGR's - the index corresponds to the grid_nr. */
  hash_type           * LGR_hash;      /* A hash of pointers to ecl_grid instances - for name based lookup of LGR. */

  double                rotation;
  double                origo[2];
  /*------------------------------:       The fields below this line are used for blocking algorithms - and not allocated by default.*/
  int                    block_dim; /* == 2 for maps and 3 for fields. 0 when not in use. */
  int                    block_size;
  int                    last_block_index;
  double_vector_type  ** values;
};





static void ecl_cell_compare(const ecl_cell_type * c1 , ecl_cell_type * c2, bool * equal) {
  int i;
  if (c1->active != c2->active)
    *equal = false;
  else {
    for (i=0; i < 8; i++)
      point_compare(c1->corner_list[i] , c2->corner_list[i] , equal);
    point_compare(c1->center , c2->center , equal);
  }
}



/*****************************************************************/


/**
   Observe that when allocating based on a GRID file not all cells are
   necessarily accessed beyond this function. In general not all cells
   will have a COORDS/CORNERS section in the GRID file.
*/


static ecl_cell_type * ecl_cell_alloc(void) {
  ecl_cell_type * cell = util_malloc(sizeof * cell , __func__);
  cell->active         = false;
  cell->lgr            = NULL;
  cell->center         = point_alloc_empty();
  for (int i=0; i < 8; i++)
    cell->corner_list[i] = point_alloc_empty();
  
  cell->host_cell      = -1;
  return cell;
}

static void ecl_cell_install_lgr( ecl_cell_type * cell , const ecl_grid_type * lgr_grid) {
  cell->lgr       = lgr_grid;
}


static void ecl_cell_fprintf( ecl_cell_type * cell , FILE * stream ) {
  int i;
  for (i=0; i < 7; i++) {
    printf("\nCorner[%d] => ",i);
    point_fprintf( cell->corner_list[i] , stdout );
  }
  fprintf(stream , "-----------------------------------\n");
}



/**
   Here comes some functions used to determine whether a certain point
   (x,y,z) is within a cell. These functions are used when blocking scalars, 
   they are not really well tested.
*/


/*
  A x B = [(Ay*Bz - Az*By) , -(Ax*Bz - Az*Bx) , (Ax*By - Bx*Ay)]
*/

/*
   Computes the signed distance from the point p the plane spanned by
   the two vectors (p1 - p0) x (p2 - p0).
*/


//static void __set_normal_vector3d(ecl_point_type * n , ecl_point_type p0 , ecl_point_type p1 , ecl_point_type p2 , bool right_hand) {
//  ecl_point_type v1 = p1;
//  ecl_point_type v2 = p2;
//  
//  ecl_point_inplace_sub(&v1 , p0);
//  ecl_point_inplace_sub(&v2 , p0);
//  n->x =  (v1.y*v2.z - v1.z*v2.y);
//  n->y = -(v1.x*v2.z - v1.z*v2.x);
//  n->z =  (v1.x*v2.y - v1.y*v2.x);
//  if (!right_hand)
//    ecl_point_inplace_scale(n , -1);
//}
//
//
//static double __signed_distance3d(ecl_point_type p0 , ecl_point_type p1 , ecl_point_type p2 , bool right_hand , ecl_point_type p) {
//  ecl_point_type n;
//  __set_normal_vector3d(&n , p0 , p1 , p2 , right_hand);
//  //ecl_point_fprintf(&n , stdout);
//  ecl_point_inplace_sub(&p  , p0);
//  {
//    double d = n.x*p.x + n.y*p.y + n.z*p.z;
//    return d;
//  }
//}
//
//static bool __positive_distance3d(ecl_point_type p0 , ecl_point_type p1 , ecl_point_type p2 , bool right_hand , ecl_point_type p) {
//  double d = __signed_distance3d(p0 , p1 , p2 , right_hand , p);
//  if (d >= 0)
//    return true;
//  else
//    return false;
//}
//
//
//
//static void __set_normal_vector2d(ecl_point_type * n , ecl_point_type p0 , ecl_point_type p1 , bool right_hand) {
//  ecl_point_type v = p1;
//
//  ecl_point_inplace_sub(&v , p0);
//  n->x = -v.y;
//  n->y =  v.x;
//  n->z =  0;
//  if (!right_hand)
//    ecl_point_inplace_scale(n , -1.0);
//
//}
//
//
//static double __signed_distance2d(ecl_point_type p0 , ecl_point_type p1 , bool right_hand , ecl_point_type p) {
//  ecl_point_type n;
//  __set_normal_vector2d(&n  ,  p0 , p1 , right_hand);
//  ecl_point_inplace_sub(&p  ,  p0);
//  {
//    double d = n.x*p.x + n.y*p.y;
//    return d;
//  }
//}
//
//static bool __positive_distance2d(ecl_point_type p0 , ecl_point_type p1 , bool right_hand , ecl_point_type p) {
//  double d = __signed_distance2d(p0 , p1 , right_hand , p);
//  if (d >= 0)
//    return true;
//  else
//    return false;
//}






/*

  6---7
  |   |
  4---5



Lower layer:

  2---3
  |   |
  0---1

*/


// Completely fucked: static void __set_normal_vector3d__(double *A , double *B , double *C, ecl_point_type p0 , ecl_point_type p1 , ecl_point_type p2) {
// Completely fucked:   ecl_point_type v1 = p1;
// Completely fucked:   ecl_point_type v2 = p2;
// Completely fucked:   
// Completely fucked:   ecl_point_inplace_sub(&v1 , p0);
// Completely fucked:   ecl_point_inplace_sub(&v2 , p0);
// Completely fucked:   *A =  (v1.y*v2.z - v1.z*v2.y);
// Completely fucked:   *B = -(v1.x*v2.z - v1.z*v2.x);
// Completely fucked:   *C =  (v1.x*v2.y - v1.y*v2.x);
// Completely fucked: }
// Completely fucked: 
// Completely fucked: 
// Completely fucked: 
// Completely fucked: static bool ecl_cell_contains_3d__(const ecl_cell_type * cell , ecl_point_type p) {
// Completely fucked:   ecl_point_type p0 = cell->corner_list[0];
// Completely fucked:   ecl_point_type p1 = cell->corner_list[1];
// Completely fucked:   ecl_point_type p2 = cell->corner_list[2];
// Completely fucked:   ecl_point_type p3 = cell->corner_list[3];
// Completely fucked:   ecl_point_type p4 = cell->corner_list[4];
// Completely fucked:   ecl_point_type p5 = cell->corner_list[5];
// Completely fucked:   ecl_point_type p6 = cell->corner_list[6];
// Completely fucked:   ecl_point_type p7 = cell->corner_list[7];
// Completely fucked: 
// Completely fucked:   /**
// Completely fucked:      Equation for plane:
// Completely fucked:      
// Completely fucked:      A*(x - a) + B*(y - b) + C*(z - c) = 0
// Completely fucked: 
// Completely fucked:      Solved with respect to:
// Completely fucked: 
// Completely fucked:        z(x,y) = (D - A*x - B*y)  / C
// Completely fucked: 
// Completely fucked:        y(z,x) = (D - A*x - C*z)  / B
// Completely fucked: 
// Completely fucked:        x(y,z) = (D - B*y - C*z ) / A
// Completely fucked: 
// Completely fucked:      Where:
// Completely fucked:      
// Completely fucked:        D = A*a + B*b + C*c
// Completely fucked:   */
// Completely fucked: 
// Completely fucked: 
// Completely fucked:   //printf("1111 \n");
// Completely fucked:   
// Completely fucked:   /**
// Completely fucked:      Z planes.
// Completely fucked:   */
// Completely fucked:   double A,B,C,D;
// Completely fucked:   {
// Completely fucked:     double z;
// Completely fucked:     __set_normal_vector3d__( &A , &B , &C , p0 , p1 , p2);
// Completely fucked:     if (fabs(A) + fabs(B) + fabs(C) == 0)
// Completely fucked:       return false;
// Completely fucked:     
// Completely fucked:     D = A*p0.x + B*p0.y + C*p0.z;
// Completely fucked:     z = (D - A * p.x - B*p.y) / C;
// Completely fucked:     //printf("D:%g  A:%g  B:%g   C:%G \n",D , A,B,C);
// Completely fucked:     //printf("Comparing: %g %g \n",z,p.z);
// Completely fucked:     //printf("Naive: %g \n",0.25*(p0.z + p1.z + p2.z + p3.z));
// Completely fucked:     //printf("recovering p2: p2.z :%g  calcultaed:%g\n",p2.z,(D - A*p2.x - B*p2.y)/C);
// Completely fucked:     printf("plane check: %g  %g \n",p3.z , (D - A*p3.x - B*p3.y)/C);
// Completely fucked:     if (z > p.z)
// Completely fucked:       return false;
// Completely fucked:     
// Completely fucked:     //printf("dz: %g %g %g %g \n",p6.z - p2.z ,p7.z - p3.z , p4.z - p0.z , p5.z - p1.z);
// Completely fucked:     //
// Completely fucked:     //printf("%g - %g \n",p0.z,p4.z);
// Completely fucked:     //printf("%g - %g \n",p1.z,p5.z);
// Completely fucked:     //printf("%g - %g \n",p2.z,p6.z);
// Completely fucked:     //printf("%g - %g \n",p3.z,p7.z);
// Completely fucked: 
// Completely fucked: 
// Completely fucked:     //printf("1111 \n");
// Completely fucked:     __set_normal_vector3d__( &A , &B , &C , p4 , p5 , p6);
// Completely fucked:     D = A*p4.x + B*p4.y + C*p4.z;
// Completely fucked:     //printf("D:%g  A:%g  B:%g   C:%G \n",D, A,B,C);
// Completely fucked:     z = (D - A * p.x - B*p.y) / C;
// Completely fucked:     //printf("Comparing: %g %g \n",z,p.z);
// Completely fucked:     //printf("Naive: %g \n",0.25*(p4.z + p5.z + p6.z + p7.z));
// Completely fucked:     //printf("recovering p5: p5.z :%g  calcultaed:%g\n",p5.z,(D - A*p5.x - B*p5.y)/C);
// Completely fucked:     if (z < p.z)
// Completely fucked:       return false;
// Completely fucked:   }
// Completely fucked: 
// Completely fucked:   
// Completely fucked:   /**
// Completely fucked:      X planes.
// Completely fucked:   */
// Completely fucked:   {
// Completely fucked:     double x;
// Completely fucked:     __set_normal_vector3d__( &A , &B , &C , p0 , p2 , p4);
// Completely fucked:     D = A*p0.x + B*p0.y + C*p0.z;
// Completely fucked:     x = (D - B * p.y - C*p.z) / A;
// Completely fucked:     if (x > p.x)
// Completely fucked:       return false;
// Completely fucked:     //printf("1111 \n");
// Completely fucked:     
// Completely fucked:     __set_normal_vector3d__( &A , &B , &C , p1 , p3 , p5);
// Completely fucked:     D = A*p1.x + B*p1.y + C*p1.z;
// Completely fucked:     x = (D - B * p.y - C*p.z) / A;
// Completely fucked:     if (x < p.x)
// Completely fucked:       return false;
// Completely fucked:   }
// Completely fucked:   //printf("1111 \n");
// Completely fucked: 
// Completely fucked:   /**
// Completely fucked:      Y planes.
// Completely fucked:   */
// Completely fucked:   {
// Completely fucked:     double y;
// Completely fucked:     __set_normal_vector3d__( &A , &B , &C , p4 , p5 , p0);
// Completely fucked:     D = A*p4.x + B*p4.y + C*p4.z;
// Completely fucked:     y = (D - A * p.x - C*p.z) / B;
// Completely fucked:     if (y > p.y)
// Completely fucked:       return false;
// Completely fucked:     //printf("1111 \n");
// Completely fucked:     
// Completely fucked:     __set_normal_vector3d__( &A , &B , &C , p6 , p7 , p2);
// Completely fucked:     D = A*p6.x + B*p6.y + C*p6.z;
// Completely fucked:     y = (D - A * p.x - C*p.z) / B;
// Completely fucked:     if (y < p.y)
// Completely fucked:       return false;
// Completely fucked:   }
// Completely fucked:   //printf("YY 1111 \n");
// Completely fucked:   return true;
// Completely fucked: }
// Completely fucked: 
// Completely fucked: 
// Completely fucked: bool ecl_grid_cell_contains1(const ecl_grid_type * grid , int global_index , double x , double y , double z) {
// Completely fucked:   ecl_point_type p;
// Completely fucked:   p.x = x;
// Completely fucked:   p.y = y;
// Completely fucked:   p.z = z;
// Completely fucked:   return ecl_cell_contains_3d__( grid->cells[global_index] , p);
// Completely fucked: }
// Completely fucked: 
// Completely fucked: 
// Completely fucked: bool ecl_grid_cell_contains3(const ecl_grid_type * grid , int i,int j,int k , double x , double y , double z) {
// Completely fucked:   ecl_point_type p;
// Completely fucked:   p.x = x;
// Completely fucked:   p.y = y;
// Completely fucked:   p.z = z;
// Completely fucked:   return ecl_cell_contains_3d__( grid->cells[ecl_grid_get_global_index3(grid , i,j,k)] , p);
// Completely fucked: }
// Completely fucked: 
// Completely fucked: 
// Completely fucked: 
// Completely fucked: static bool ecl_cell_contains_3d(const ecl_cell_type * cell , ecl_point_type p) {
// Completely fucked:   ecl_point_type p0 = cell->corner_list[0];
// Completely fucked:   ecl_point_type p1 = cell->corner_list[1];
// Completely fucked:   ecl_point_type p2 = cell->corner_list[2];
// Completely fucked:   ecl_point_type p3 = cell->corner_list[3];
// Completely fucked:   ecl_point_type p4 = cell->corner_list[4];
// Completely fucked:   ecl_point_type p5 = cell->corner_list[5];
// Completely fucked:   ecl_point_type p6 = cell->corner_list[6];
// Completely fucked:   ecl_point_type p7 = cell->corner_list[7];
// Completely fucked:   bool  contains = false;
// Completely fucked: 
// Completely fucked:   /**
// Completely fucked:      Rectangular approximation - this whole thing might be fucked by MAPAXES.
// Completely fucked:   */
// Completely fucked:   
// Completely fucked:   {
// Completely fucked:     /* Of all fucking heuristics .... */
// Completely fucked:     const double max_xy = 2500;
// Completely fucked:     const double max_z  = 250;
// Completely fucked:     
// Completely fucked:     double z1 = 0.25 * (p0.z + p1.z + p2.z + p3.z);
// Completely fucked:     double z2 = 0.25 * (p4.z + p5.z + p6.z + p7.z);
// Completely fucked: 
// Completely fucked:     double y1 = 0.25 * (p0.y + p1.y + p4.y + p5.y);
// Completely fucked:     double y2 = 0.25 * (p2.y + p3.y + p6.y + p7.y);
// Completely fucked: 
// Completely fucked:     double x1 = 0.25* (p0.x + p2.x + p4.x + p6.x);
// Completely fucked:     double x2 = 0.25* (p1.x + p3.x + p7.x + p5.x);
// Completely fucked:     
// Completely fucked:     double zmax = util_double_max(z1 , z2);
// Completely fucked:     double zmin = util_double_min(z1 , z2);
// Completely fucked: 
// Completely fucked:     double xmax = util_double_max(x1 , x2);
// Completely fucked:     double xmin = util_double_min(x1 , x2);
// Completely fucked:     
// Completely fucked:     double ymax = util_double_max(y1 , y2);
// Completely fucked:     double ymin = util_double_min(y1 , y2);
// Completely fucked:  
// Completely fucked: 
// Completely fucked:     if (((xmax - xmin) < max_xy) && 
// Completely fucked:         ((ymax - ymin) < max_xy) && 
// Completely fucked:         ((zmax - zmin) < max_z)) {
// Completely fucked: 
// Completely fucked:       if (((zmin <= p.z) && (zmax >= p.z)) &&
// Completely fucked:           ((ymin <= p.y) && (ymax >= p.y)) &&
// Completely fucked:           ((xmin <= p.x) && (xmax >= p.x)))
// Completely fucked:         
// Completely fucked:         contains = true;
// Completely fucked:     }
// Completely fucked: 
// Completely fucked:     //if (contains) {
// Completely fucked:     //  printf(" X: %g < %g <%g \n",xmin , p.x, xmax);
// Completely fucked:     //  printf(" Y: %g < %g <%g \n",ymin , p.y, ymax);
// Completely fucked:     //  printf(" Z: %g < %g <%g \n",zmin , p.z, zmax);
// Completely fucked:     //}
// Completely fucked:   }
// Completely fucked:   return contains;
// Completely fucked: }
// Completely fucked:   
// Completely fucked: 
// Completely fucked: //static bool ecl_cell_contains_3d(const ecl_cell_type * cell , ecl_point_type p) {
// Completely fucked: //  ecl_point_type p0 = cell->corner_list[0];
// Completely fucked: //  ecl_point_type p1 = cell->corner_list[1];
// Completely fucked: //  ecl_point_type p2 = cell->corner_list[2];
// Completely fucked: //  ecl_point_type p3 = cell->corner_list[3];
// Completely fucked: //  ecl_point_type p4 = cell->corner_list[4];
// Completely fucked: //  ecl_point_type p5 = cell->corner_list[5];
// Completely fucked: //  ecl_point_type p6 = cell->corner_list[6];
// Completely fucked: //  //ecl_point_type p7 = cell->corner_list[7];
// Completely fucked: //  bool  contains = false;
// Completely fucked: //
// Completely fucked: //
// Completely fucked: //  /** 
// Completely fucked: //      Calculate the surface of the base area.  This can be zero for funny
// Completely fucked: //      aquifer cells, which will fool the algorithm completely.
// Completely fucked: //  */
// Completely fucked: //  double surface_area = ((p1.x - p0.x) * (p2.y - p0.y) - ((p1.y - p0.y) * (p2.x - p0.x)));
// Completely fucked: //  
// Completely fucked: //  if (surface_area > 0) {
// Completely fucked: //    if (__positive_distance3d(p0 , p1 , p2 , true , p))        	  /* Z1 */
// Completely fucked: //      if (__positive_distance3d(p4 , p5 , p6 , false , p))     	  /* Z2 */
// Completely fucked: //        if (__positive_distance3d(p0 , p4 , p2 , true , p))    	  /* X1 */
// Completely fucked: //          if (__positive_distance3d(p1 , p5 , p3 , false , p)) 	  /* X2 */
// Completely fucked: //            if (__positive_distance3d(p0 , p4 , p1 , true , p))     /* Y1 */
// Completely fucked: //              if (__positive_distance3d(p2 , p6 , p3 , false , p))  /* Y2 */
// Completely fucked: //                contains = true;
// Completely fucked: //  }
// Completely fucked: //  return contains;
// Completely fucked: //}
// Completely fucked: 
// Completely fucked: 
// Completely fucked: static bool ecl_cell_contains_2d(const ecl_cell_type * cell , ecl_point_type p) {
// Completely fucked:   bool contains = false;
// Completely fucked:   ecl_point_type p0 = cell->corner_list[0];
// Completely fucked:   ecl_point_type p1 = cell->corner_list[1];
// Completely fucked:   ecl_point_type p2 = cell->corner_list[2];
// Completely fucked:   ecl_point_type p3 = cell->corner_list[3];
// Completely fucked: 
// Completely fucked:   if (__positive_distance2d(p0 , p2 , false , p))
// Completely fucked:     if (__positive_distance2d(p0 , p1 , true , p))
// Completely fucked:       if (__positive_distance2d(p1 , p3 , true , p))
// Completely fucked: 	if (__positive_distance2d(p2 , p3 , false , p))
// Completely fucked: 	  contains = true;
// Completely fucked: 
// Completely fucked:   return contains;
// Completely fucked: }




static void ecl_cell_free(ecl_cell_type * cell) {
  point_free( cell->center );
  {
    int i;
    for (i=0; i < 8; i++)
      point_free( cell->corner_list[i] );
  }
  free(cell);
}

/* End of cell implementation                                    */
/*****************************************************************/
/* Starting on the ecl_grid proper implementation                */

UTIL_SAFE_CAST_FUNCTION(ecl_grid , ECL_GRID_ID);

/**
   This function allocates the internal index_map and inv_index_map fields.
*/

static void ecl_grid_alloc_index_map(ecl_grid_type * ecl_grid) {
  int * index_map     = util_malloc(ecl_grid->size         * sizeof * index_map     , __func__);
  int * inv_index_map = util_malloc(ecl_grid->total_active * sizeof * inv_index_map , __func__);
  int   index;

  for (index = 0; index < ecl_grid->size; index++) {
    const ecl_cell_type * cell = ecl_grid->cells[index];
    if (cell->active) {
      index_map[index] = cell->active_index;
      inv_index_map[cell->active_index] = index;
    } else
      index_map[index] = -1;
  }

  ecl_grid->inv_index_map = inv_index_map;
  ecl_grid->index_map     = index_map;
}



static ecl_grid_type * ecl_grid_alloc_empty(int nx , int ny , int nz, int grid_nr) {
  ecl_grid_type * grid = util_malloc(sizeof * grid , __func__);
  UTIL_TYPE_ID_INIT(grid , ECL_GRID_ID);
  grid->nx = nx;
  grid->ny = ny;
  grid->nz = nz;
  grid->size    = nx*ny*nz;
  grid->grid_nr = grid_nr;

  grid->inv_index_map = NULL;
  grid->index_map     = NULL;
  grid->cells         = util_malloc(nx*ny*nz * sizeof * grid->cells , __func__);
  grid->rotation      = 0;
  grid->origo[0]      = 0;
  grid->origo[1]      = 0;
  {
    int i;
    for (i=0; i < grid->size; i++)
      grid->cells[i] = ecl_cell_alloc();
  }
  grid->block_dim      = 0;
  grid->values         = NULL;
  if (grid_nr == 0) {  /* This is the main grid */
    grid->LGR_list = vector_alloc_new(); 
    vector_append_ref( grid->LGR_list , grid ); /* Adding a 'self' pointer as first inistance - without destructor! */
    grid->LGR_hash = hash_alloc();
  } else {
    grid->LGR_list = NULL;
    grid->LGR_hash = NULL;
  }
  grid->parent_name = NULL;
  grid->parent_grid = NULL;
  grid->children    = hash_alloc();
  return grid;
}


static void ecl_grid_set_center(ecl_grid_type * ecl_grid) {
  int c , i;
  for (i=0; i < ecl_grid->size; i++) {
    ecl_cell_type * cell = ecl_grid->cells[i];
    point_set(cell->center , 0,0,0);
    for (c = 0; c < 8; c++)
      point_inplace_add(cell->center , cell->corner_list[c]);
    point_inplace_scale(cell->center , 1.0 / 8.0);
  }
}


static inline int ecl_grid_get_global_index__(const ecl_grid_type * ecl_grid , int i , int j , int k) {
  return i + j * ecl_grid->nx + k * ecl_grid->nx * ecl_grid->ny;
}


static void ecl_grid_set_cell_EGRID(ecl_grid_type * ecl_grid , int i, int j , int k , double x[4][2] , double y[4][2] , double z[4][2] , const int * actnum) {

  const int global_index   = ecl_grid_get_global_index__(ecl_grid , i , j  , k );
  ecl_cell_type * cell     = ecl_grid->cells[global_index];
  int ip , iz;

  for (iz = 0; iz < 2; iz++) {
    for (ip = 0; ip < 4; ip++) {
      int c = ip + iz * 4;
      point_set(cell->corner_list[c] , x[ip][iz] , y[ip][iz] , z[ip][iz]);
    }
  }


  /*
    For normal runs actnum will be 1 for active cells,
    for dual porosity models it can also be 2 and 3.
  */
  if (actnum[global_index] > 0)
    cell->active = true;
}


static void ecl_grid_set_cell_GRID(ecl_grid_type * ecl_grid , const ecl_kw_type * coords_kw , const ecl_kw_type * corners_kw) {
  const int   * coords  = ecl_kw_get_int_ptr(coords_kw);
  const float * corners = ecl_kw_get_float_ptr(corners_kw);
  const int i  = coords[0]; /* ECLIPSE 1 offset */
  const int j  = coords[1];
  const int k  = coords[2];
  const int global_index   = ecl_grid_get_global_index__(ecl_grid , i - 1, j - 1 , k - 1);
  ecl_cell_type * cell     = ecl_grid->cells[global_index];

  /* The coords keyword can optionally contain 4,5 or 7 elements:

     	coords[0..2] = i,j,k
     	coords[3]    = global_cell number (not used here)
     	----
     	coords[4]    = 1,0 for active/inactive cells
     	coords[5]    = 0 for normal cells, icell of host cell for LGR cell.
	coords[6]    = 0 for normal cells, coarsening group for coarsened cell [NOT TREATED YET].

     If coords[4] is not present it is assumed that the cell is active.
  */

  {
    int c;
    int coords_size = ecl_kw_get_size(coords_kw);

    switch(coords_size) {
    case(4):                /* All cells active */
      cell->active = true;
      break;
    case(5):                /* Only spesific cells active - no LGR */
      cell->active  = (coords[4] == 1) ? true : false;
      break;
    case(7):
      cell->active    = (coords[4] == 1) ? true : false;
      cell->host_cell = coords[5];
      break;
    }
    
    for (c = 0; c < 8; c++)
      point_set(cell->corner_list[c] , corners[3*c] , corners[3*c + 1] , corners[3*c + 2]);
    
  }
}



static void ecl_grid_set_active_index(ecl_grid_type * ecl_grid) {
  int i,j,k;
  int active_index = 0;

  for (k=0; k < ecl_grid->nz; k++)
    for (j=0; j < ecl_grid->ny; j++)
      for (i=0; i < ecl_grid->nx; i++) {
	const int global_index   = ecl_grid_get_global_index__(ecl_grid , i , j , k );
	ecl_cell_type * cell     = ecl_grid->cells[global_index];
	if (cell->active) {
	  cell->active_index = active_index;
	  active_index++;
	} else
	  cell->active_index = -1;
      }

  ecl_grid->total_active = active_index;
}



static void ecl_grid_pillar_cross_planes(const point_type * pillar , const double *z , double *x , double *y) {
  double e_x , e_y , e_z;
  int k;
  e_x = pillar[1].x - pillar[0].x;
  e_y = pillar[1].y - pillar[0].y;
  e_z = pillar[1].z - pillar[0].z;

  for (k=0; k < 2; k++) {
    double t = (z[k] -  pillar[0].z) / e_z;
    x[k] = pillar[0].x + t * e_x;
    y[k] = pillar[0].y + t * e_y;
  }
}


static void ecl_grid_init_mapaxes( ecl_grid_type * ecl_grid , const float * mapaxes) {
  ecl_grid->rotation = 0;
  ecl_grid->origo[0] = 0;
  ecl_grid->origo[1] = 0;
}





/**
   This function will add a ecl_grid instance as a LGR to the main
   grid. The LGR grid as added to two different structures of the main
   grid:

    1. In the main_grid->LGR_list the LGR instances are inserted in
       order of occurence in the GRID file. The following equalities
       should apply:

          occurence number in file == lgr_grid->grid_nr == GRIDHEAD(4) for lgr == index in the LGR_list vector
  
       When installed in the LGR_list vector the lgr grid is installed
       with a destructor, i.e. the grid is destroyed when the vector
       is destroyed.

    2. In the main->LGR_hash the lgr instance is installed with the
       LGRNAME as key. Only a reference is installed in the hash
       table. 

    Observe that this is in principle somewhat different from the
    install functions below; here the lgr is added to the top level
    grid (i.e. the main grid) which has the storage responsability of
    all the lgr instances. The cell->lgr relationship is established
    in the _install_EGRID / install_GRID functions further down.
*/


static void ecl_grid_add_lgr( ecl_grid_type * main_grid , const ecl_grid_type * lgr_grid) {
  int next_grid_nr = vector_get_size( main_grid->LGR_list);
  if (next_grid_nr != lgr_grid->grid_nr) 
    util_abort("%s: index based insertion of LGR grid failed. next_grid_nr:%d  lgr->grid_nr:%d \n",__func__ , next_grid_nr , lgr_grid->grid_nr);
  {
    vector_append_owned_ref( main_grid->LGR_list , lgr_grid , ecl_grid_free__);
    hash_insert_ref( main_grid->LGR_hash , lgr_grid->name , lgr_grid);
  }
}



/**
   This function will set the lgr pointer of the relevant cells in the
   host grid to point to the lgr_grid. Observe that the ecl_cell_type
   instances do *NOT* own the lgr_grid - all lgr_grid instances are
   owned by the main grid.
*/

static void ecl_grid_install_lgr_EGRID(ecl_grid_type * host_grid , ecl_grid_type * lgr_grid , const int * hostnum) {
  int global_lgr_index;

  for (global_lgr_index = 0; global_lgr_index < lgr_grid->size; global_lgr_index++) {
    ecl_cell_type * lgr_cell = lgr_grid->cells[global_lgr_index];
    if (lgr_cell->active) {
      ecl_cell_type * host_cell = host_grid->cells[ hostnum[ global_lgr_index ] ];
      ecl_cell_install_lgr( host_cell , lgr_grid );

      lgr_cell->host_cell = hostnum[ global_lgr_index ];
    }
  }
  hash_insert_ref( host_grid->children , lgr_grid->name , lgr_grid);
  lgr_grid->parent_grid = host_grid;
}


/**
   Similar to ecl_grid_install_lgr_EGRID for GRID based instances. 
*/
static void ecl_grid_install_lgr_GRID(ecl_grid_type * host_grid , const ecl_grid_type * lgr_grid) {
  int global_lgr_index;
  
  for (global_lgr_index = 0; global_lgr_index < lgr_grid->size; global_lgr_index++) {
    ecl_cell_type * lgr_cell = lgr_grid->cells[global_lgr_index];
    if (lgr_cell->active) {
      ecl_cell_type * host_cell = host_grid->cells[ lgr_cell->host_cell ];
      ecl_cell_install_lgr( host_cell , lgr_grid );
    }
  }
}



/**
   Sets the name of the lgr AND the name of the parent, if this is a
   nested LGR. For normal LGR descending directly from the coarse grid
   the parent_name is set to NULL.
*/
   

static void ecl_grid_set_lgr_name_EGRID(ecl_grid_type * lgr_grid , const ecl_file_type * ecl_file , int grid_nr) {
  ecl_kw_type * lgrname_kw = ecl_file_iget_named_kw( ecl_file , "LGR" , grid_nr - 1);
  lgr_grid->name = util_alloc_strip_copy( ecl_kw_iget_ptr( lgrname_kw , 0) );  /* Trailing zeros are stripped away. */
  if (ecl_file_has_kw( ecl_file , "LGRPARNT")) {
    ecl_kw_type * parent_kw = ecl_file_iget_named_kw( ecl_file , "LGRPARNT" , grid_nr - 1);
    char * parent = util_alloc_strip_copy( ecl_kw_iget_ptr( parent_kw , 0));

    if (strlen( parent ) > 0) 
      lgr_grid->parent_name = parent;
    else  /* lgr_grid->parent has been initialized to NULL */
      free( parent );
  }
}

/**
   Sets the name of the lgr AND the name of the parent, if this is a
   nested LGR. For LGR descending directly from the parent ECLIPSE
   will supply 'GLOBAL' (whereas for EGRID it will return '' -
   cool?). Anyway GLOBAL -> NULL.
*/

static void ecl_grid_set_lgr_name_GRID(ecl_grid_type * lgr_grid , const ecl_file_type * ecl_file , int grid_nr) {
  ecl_kw_type * lgr_kw = ecl_file_iget_named_kw( ecl_file , "LGR" , grid_nr - 1);
  lgr_grid->name = util_alloc_strip_copy( ecl_kw_iget_ptr( lgr_kw , 0) );  /* Trailing zeros are stripped away. */
  {
    char * parent = util_alloc_strip_copy( ecl_kw_iget_ptr( lgr_kw , 1));
    if ((strlen(parent) == 0) || (strcmp(parent , "GLOBAL") == 0))
      free( parent );
    else
      lgr_grid->parent_name = parent;
  }
}


/*
  2---3
  |   |
  0---1
*/

static ecl_grid_type * ecl_grid_alloc_GRDECL__(int nx , int ny , int nz , const float * zcorn , const float * coord , const int * actnum, const float * mapaxes, int grid_nr) {
  int i,j,k;
  ecl_grid_type * ecl_grid = ecl_grid_alloc_empty(nx,ny,nz,grid_nr);
  point_type pillars[4][2];

  for (j=0; j < ny; j++) {
    for (i=0; i < nx; i++) {
      int pillar_index[4];
      int ip;
      pillar_index[0] = 6 * ( j      * (nx + 1) + i    );
      pillar_index[1] = 6 * ( j      * (nx + 1) + i + 1);
      pillar_index[2] = 6 * ((j + 1) * (nx + 1) + i    );
      pillar_index[3] = 6 * ((j + 1) * (nx + 1) + i + 1);

      for (ip = 0; ip < 4; ip++) {
        int index = pillar_index[ip];
	point_set(&pillars[ip][0] , coord[index] , coord[index + 1] , coord[index + 2]);
        
        index += 3;
	point_set(&pillars[ip][1] , coord[index] , coord[index + 1] , coord[index + 2]);
      }


      for (k=0; k < nz; k++) {
	double x[4][2];
	double y[4][2];
	double z[4][2];
	int c;

	for (c = 0; c < 2; c++) {
	  z[0][c] = zcorn[k*8*nx*ny + j*4*nx + 2*i            + c*4*nx*ny];
	  z[1][c] = zcorn[k*8*nx*ny + j*4*nx + 2*i  +  1      + c*4*nx*ny];
	  z[2][c] = zcorn[k*8*nx*ny + j*4*nx + 2*nx + 2*i     + c*4*nx*ny];
	  z[3][c] = zcorn[k*8*nx*ny + j*4*nx + 2*nx + 2*i + 1 + c*4*nx*ny];
	}

	for (ip = 0; ip <  4; ip++)
	  ecl_grid_pillar_cross_planes(pillars[ip] , z[ip] , x[ip] , y[ip]);

	ecl_grid_set_cell_EGRID(ecl_grid , i , j , k , x , y , z , actnum);
      }
    }
  }
  if (mapaxes != NULL)
    ecl_grid_init_mapaxes( ecl_grid , mapaxes );
    
  ecl_grid_set_center(ecl_grid);
  ecl_grid_set_active_index(ecl_grid);
  ecl_grid_alloc_index_map(ecl_grid);
  return ecl_grid;
}


ecl_grid_type * ecl_grid_alloc_GRDECL(int nx , int ny , int nz , const float * zcorn , const float * coord , const int * actnum, const float * mapaxes) {
  return ecl_grid_alloc_GRDECL__(nx , ny , nz , zcorn , coord , actnum , mapaxes , 0);
}


static ecl_grid_type * ecl_grid_alloc_EGRID__(const char * grid_file , const ecl_file_type * ecl_file , int grid_nr) {
  ecl_kw_type * gridhead_kw = ecl_file_iget_named_kw( ecl_file , "GRIDHEAD" , grid_nr);
  int gtype, nx,ny,nz;

  gtype   = ecl_kw_iget_int(gridhead_kw , 0);
  nx 	  = ecl_kw_iget_int(gridhead_kw , 1);
  ny 	  = ecl_kw_iget_int(gridhead_kw , 2);
  nz 	  = ecl_kw_iget_int(gridhead_kw , 3);
  if (gtype != 1)
    util_abort("%s: gtype:%d fatal error when loading:%s - must have corner point grid - aborting\n",__func__ , gtype , grid_file);
  {
    
    ecl_kw_type * zcorn_kw     = ecl_file_iget_named_kw( ecl_file , "ZCORN"   	, grid_nr);
    ecl_kw_type * coord_kw     = ecl_file_iget_named_kw( ecl_file , "COORD"   	, grid_nr);
    ecl_kw_type * actnum_kw    = ecl_file_iget_named_kw( ecl_file , "ACTNUM"  	, grid_nr);
    float       * mapaxes_data = NULL;
    
    if ((grid_nr == 0) && (ecl_file_has_kw( ecl_file , "MAPAXES"))) {
      const ecl_kw_type * mapaxes_kw   = ecl_file_iget_named_kw( ecl_file , "MAPAXES" , grid_nr);
      mapaxes_data = ecl_kw_get_float_ptr( mapaxes_kw );
    }
    
    ecl_grid_type * ecl_grid = ecl_grid_alloc_GRDECL__(nx , ny , nz , ecl_kw_get_float_ptr(zcorn_kw) , ecl_kw_get_float_ptr(coord_kw) , ecl_kw_get_int_ptr(actnum_kw) , mapaxes_data, grid_nr);
    if (grid_nr > 0) ecl_grid_set_lgr_name_EGRID(ecl_grid , ecl_file , grid_nr);
    return ecl_grid;
  }
}



static ecl_grid_type * ecl_grid_alloc_EGRID(const char * grid_file ) {
  ecl_file_enum   file_type;
  bool            fmt_file;
  ecl_util_get_file_type(grid_file , &file_type , &fmt_file , NULL);
  if (file_type != ECL_EGRID_FILE)
    util_abort("%s: %s wrong file type - expected .EGRID file - aborting \n",__func__ , grid_file);
  {
    ecl_file_type * ecl_file   = ecl_file_fread_alloc( grid_file );
    int num_grid               = ecl_file_get_num_named_kw( ecl_file , "GRIDHEAD" );
    ecl_grid_type * main_grid  = ecl_grid_alloc_EGRID__( grid_file , ecl_file , 0);
    
    for (int grid_nr = 1; grid_nr < num_grid; grid_nr++) {
      ecl_grid_type * lgr_grid = ecl_grid_alloc_EGRID__(grid_file , ecl_file , grid_nr );
      ecl_grid_add_lgr( main_grid , lgr_grid );
      {
    	ecl_grid_type * host_grid;
    	ecl_kw_type   * hostnum_kw = ecl_file_iget_named_kw( ecl_file , "HOSTNUM" , grid_nr - 1);
    	if (lgr_grid->parent_name == NULL)
    	  host_grid = main_grid;
    	else 
    	  host_grid = ecl_grid_get_lgr( main_grid , lgr_grid->parent_name );
    	  
    	ecl_grid_install_lgr_EGRID( host_grid , lgr_grid , ecl_kw_get_int_ptr( hostnum_kw) );
      }
    }
    ecl_file_free( ecl_file );
    return main_grid;
  }
}










/* 
   
*/

static ecl_grid_type * ecl_grid_alloc_GRID__(const char * file , const ecl_file_type * ecl_file , int * cell_offset , int grid_nr) {
  int index,nx,ny,nz;
  ecl_grid_type * grid;
  ecl_kw_type * dimens_kw   = ecl_file_iget_named_kw( ecl_file , "DIMENS" , grid_nr);
  nx   = ecl_kw_iget_int(dimens_kw , 0);
  ny   = ecl_kw_iget_int(dimens_kw , 1);
  nz   = ecl_kw_iget_int(dimens_kw , 2);
  grid = ecl_grid_alloc_empty(nx , ny , nz, grid_nr);
  
  /*
    Possible LGR cells will follow *AFTER* the first nx*ny*nz cells;
    the loop stops at nx*ny*nz. Additionally the LGR cells should be
    discarded (by checking coords[5]) in the
    ecl_grid_set_cell_GRID() function.
  */
    
  for (index = 0; index < nx*ny*nz; index++) {
    ecl_kw_type * coords_kw  = ecl_file_iget_named_kw(ecl_file , "COORDS"  , index + (*cell_offset));
    ecl_kw_type * corners_kw = ecl_file_iget_named_kw(ecl_file , "CORNERS" , index + (*cell_offset));
    ecl_grid_set_cell_GRID(grid , coords_kw , corners_kw);
  }
  
  if ((grid_nr == 0) && (ecl_file_has_kw( ecl_file , "MAPAXES"))) {
    const ecl_kw_type * mapaxes_kw = ecl_file_iget_named_kw( ecl_file , "MAPAXES" , grid_nr);
    ecl_grid_init_mapaxes( grid , ecl_kw_get_float_ptr( mapaxes_kw) );
  }
  
  ecl_grid_set_center(grid);
  ecl_grid_set_active_index(grid);
  (*cell_offset) += nx*ny*nz;
  ecl_grid_alloc_index_map(grid);
  if (grid_nr > 0) ecl_grid_set_lgr_name_GRID(grid , ecl_file , grid_nr);
  return grid;
}



static ecl_grid_type * ecl_grid_alloc_GRID(const char * grid_file) {

  ecl_file_enum   file_type;
  ecl_util_get_file_type(grid_file , &file_type , NULL , NULL);
  if (file_type != ECL_GRID_FILE)
    util_abort("%s: %s wrong file type - expected .GRID file - aborting \n",__func__ , grid_file);

  {
    int cell_offset = 0;
    ecl_file_type * ecl_file  = ecl_file_fread_alloc( grid_file );
    int num_grid              = ecl_file_get_num_named_kw( ecl_file , "DIMENS");
    ecl_grid_type * main_grid = ecl_grid_alloc_GRID__(grid_file , ecl_file , &cell_offset , 0);
    for (int grid_nr = 1; grid_nr < num_grid; grid_nr++) {
      ecl_grid_type * lgr_grid = ecl_grid_alloc_GRID__(grid_file , ecl_file , &cell_offset , grid_nr );
      ecl_grid_add_lgr( main_grid , lgr_grid );
      {
      	ecl_grid_type * host_grid;
      	if (lgr_grid->parent_name == NULL)
      	  host_grid = main_grid;
      	else 
      	  host_grid = ecl_grid_get_lgr( main_grid , lgr_grid->parent_name );
      	  
      	ecl_grid_install_lgr_GRID( host_grid , lgr_grid );
      }
    }

    ecl_file_free( ecl_file );
    return main_grid;
  }
}
				 



/**
   This function will allocate a ecl_grid instance. As input it takes
   a filename, which can be both a GRID file and an EGRID file (both
   formatted and unformatted).

   When allocating based on an EGRID file the COORDS, ZCORN and ACTNUM
   keywords are extracted, and the ecl_grid_alloc_GRDECL() function is
   called with these keywords. This function can be called directly
   with these keywords.
*/

ecl_grid_type * ecl_grid_alloc(const char * grid_file ) {
  ecl_file_enum    file_type;
  bool             fmt_file;
  ecl_grid_type  * ecl_grid = NULL;

  ecl_util_get_file_type(grid_file , &file_type , &fmt_file , NULL);
  if (file_type == ECL_GRID_FILE)
    ecl_grid = ecl_grid_alloc_GRID(grid_file );
  else if (file_type == ECL_EGRID_FILE)
    ecl_grid = ecl_grid_alloc_EGRID(grid_file);
  else
    util_abort("%s must have .GRID or .EGRID file - %s not recognized \n", __func__ , grid_file);
  
  ecl_grid->name = util_alloc_string_copy( grid_file );
  return ecl_grid;
}



/**
   Return true if grids g1 and g2 are equal, and false otherwise. To
   return true all cells must be identical.
*/

bool ecl_grid_compare(const ecl_grid_type * g1 , const ecl_grid_type * g2) {
  int i;

  bool equal = true;
  if (g1->size != g2->size)
    equal = false;
  else {
    for (i = 0; i < g1->size; i++) {
      ecl_cell_type *c1 = g1->cells[i];
      ecl_cell_type *c2 = g2->cells[i];
      ecl_cell_compare(c1 , c2 , &equal);
    }
  }
  
  return equal;
}



/*****************************************************************/
/** 
    Here comes some functions used when blocking. These are NOT used
    by default. Observe that the functions used to look up an index
    based on xy and xyz are NOT well tested.

    Will return -1 if no cell is found.
*/


static int ecl_grid_get_global_index_from_xyz__(const ecl_grid_type * grid , double x , double y , double z , int last_index) {
  int global_index = -1;
  point_type p;
  point_set( &p , x , y , z);
  util_exit("%s: Sorry - not implmenetd \n" , __func__);
  {
    int index    = 0;
    bool cont    = true;
    global_index = -1;

    do {
      int active_index = ((index + last_index) % grid->size);
      bool cell_contains;
      cell_contains = false; //ecl_cell_contains_3d__(grid->cells[active_index] , p);
      
      if (cell_contains) {
	global_index = active_index;
	cont = false;
      }
      index++;
      if (index == grid->size)
	cont = false;
    } while (cont);
  }
  return global_index;
}


int ecl_grid_get_global_index_from_xyz(const ecl_grid_type * grid , double x , double y , double z) {
  int start_index = ecl_grid_get_global_index3( grid , 10 , 10 , 10 );
  return ecl_grid_get_global_index_from_xyz__( grid , x , y , z , start_index );
}



static int ecl_grid_get_global_index_from_xy__(const ecl_grid_type * grid , double x , double y , int last_index) {
  util_exit("%s: not implemented ... \n");
  //int global_index;
  //ecl_point_type p;
  //p.x = x;
  //p.y = y;
  //p.z = -1;
  //{
  //  int index    = 0;
  //  bool cont    = true;
  //  global_index = -1;
  //
  //  do {
  //    int active_index = ((index + last_index) % grid->block_size);
  //    bool cell_contains;
  //    cell_contains = ecl_cell_contains_2d(grid->cells[active_index] , p);
  //
  //    if (cell_contains) {
  //      global_index = active_index;
  //      cont = false;
  //    }
  //    index++;
  //    if (index == grid->block_size)
  //      cont = false;
  //  } while (cont);
  //}
  //return global_index;
  return -1;
}



void ecl_grid_alloc_blocking_variables(ecl_grid_type * grid, int block_dim) {
  int index;
  grid->block_dim = block_dim;
  if (block_dim == 2)
    grid->block_size = grid->nx* grid->ny;
  else if (block_dim == 3)
    grid->block_size = grid->size;
  else
    util_abort("%: valid values are two and three. Value:%d invaid \n",__func__ , block_dim);

  grid->values         = util_malloc( grid->block_size * sizeof * grid->values , __func__);
  for (index = 0; index < grid->block_size; index++)
    grid->values[index] = double_vector_alloc( 0 , 0.0 );
}



void ecl_grid_init_blocking(ecl_grid_type * grid) {
  int index;
  for (index = 0; index < grid->block_size; index++)
    double_vector_reset(grid->values[index]);
  grid->last_block_index = 0;
}




bool ecl_grid_block_value_3d(ecl_grid_type * grid, double x , double y , double z , double value) {
  if (grid->block_dim != 3)
    util_abort("%s: Wrong blocking dimension \n",__func__);
  {
    int global_index = ecl_grid_get_global_index_from_xyz__( grid , x , y , z , grid->last_block_index);
    if (global_index >= 0) {
      double_vector_append( grid->values[global_index] , value);
      grid->last_block_index = global_index;
      return true;
    } else
      return false;
  }
}



bool ecl_grid_block_value_2d(ecl_grid_type * grid, double x , double y ,double value) {
  if (grid->block_dim != 2)
    util_abort("%s: Wrong blocking dimension \n",__func__);
  {
    int global_index = ecl_grid_get_global_index_from_xy__( grid , x , y , grid->last_block_index);
    if (global_index >= 0) {
      double_vector_append( grid->values[global_index] , value);
      grid->last_block_index = global_index;
      return true;
    } else
      return false;
  }
}



double ecl_grid_block_eval2d(ecl_grid_type * grid , int i, int j , block_function_ftype * blockf ) {
  int global_index = ecl_grid_get_global_index3(grid , i,j,0);
  return blockf( grid->values[global_index]);
}


double ecl_grid_block_eval3d(ecl_grid_type * grid , int i, int j , int k ,block_function_ftype * blockf ) {
  int global_index = ecl_grid_get_global_index3(grid , i,j,k);
  return blockf( grid->values[global_index]);
}

int ecl_grid_get_block_count2d(const ecl_grid_type * grid , int i , int j) {
  int global_index = ecl_grid_get_global_index3(grid , i,j,0);
  return double_vector_size( grid->values[global_index]);
}


int ecl_grid_get_block_count3d(const ecl_grid_type * grid , int i , int j, int k) {
  int global_index = ecl_grid_get_global_index3(grid , i,j,k);
  return double_vector_size( grid->values[global_index]);
}

/* End of blocking functions                                     */
/*****************************************************************/

void ecl_grid_free(ecl_grid_type * grid) {
  int i;
  for (i=0; i < grid->size; i++)
    ecl_cell_free(grid->cells[i]);
  free(grid->cells);
  util_safe_free(grid->index_map);
  util_safe_free(grid->inv_index_map);

  if (grid->values != NULL) {
    int i;
    for (i=0; i < grid->block_size; i++)
      double_vector_free( grid->values[i] );
    free( grid->values );
  }
  if (grid->grid_nr == 0) { /* This is the main grid. */
    vector_free( grid->LGR_list );
    hash_free( grid->LGR_hash );
  }
  hash_free( grid->children );
  util_safe_free( grid->parent_name);
  free( grid->name );
  free( grid );
}


void ecl_grid_free__( void * arg ) {
  ecl_grid_type * ecl_grid = ecl_grid_safe_cast( arg );
  ecl_grid_free( ecl_grid );
}




void ecl_grid_get_distance(const ecl_grid_type * grid , int global_index1, int global_index2 , double *dx , double *dy , double *dz) {
  const ecl_cell_type * cell1 = grid->cells[global_index1];
  const ecl_cell_type * cell2 = grid->cells[global_index2];
  
  *dx = cell1->center->x - cell2->center->x;
  *dy = cell1->center->y - cell2->center->y;
  *dz = cell1->center->z - cell2->center->z;

}



/*****************************************************************/
/* Index based query functions */
/*****************************************************************/



/**
   Only checks that i,j,k are in the required intervals:
  
      0 <= i < nx
      0 <= j < ny
      0 <= k < nz

*/
   
inline bool ecl_grid_ijk_valid(const ecl_grid_type * grid , int i , int j , int k) {
  bool OK = false;

  if (i >= 0 && i < grid->nx)
    if (j >= 0 && j < grid->ny)
      if (k >= 0 && k < grid->nz)
	OK = true;

  return OK;
}


void ecl_grid_get_dims(const ecl_grid_type * grid , int *nx , int * ny , int * nz , int * active_size) {
  if (nx != NULL) *nx 	       		= grid->nx;
  if (ny != NULL) *ny 	       		= grid->ny;
  if (nz != NULL) *nz 	       		= grid->nz;
  if (active_size != NULL) *active_size = grid->total_active;
}

int ecl_grid_get_nz( const ecl_grid_type * grid ) {
  return grid->nz;
}

int ecl_grid_get_nx( const ecl_grid_type * grid ) {
  return grid->nx;
}

int ecl_grid_get_ny( const ecl_grid_type * grid ) {
  return grid->ny;
}



/*****************************************************************/
/* Functions for converting between the different index types. */

/**
   Converts: (i,j,k) -> global_index. i,j,k are zero offset.
*/

int ecl_grid_get_global_index3(const ecl_grid_type * ecl_grid , int i , int j , int k) {
  if (ecl_grid_ijk_valid(ecl_grid , i , j , k))
    return ecl_grid_get_global_index__(ecl_grid , i , j , k);
  else {
    util_abort("%s: i,j,k = (%d,%d,%d) is invalid:\n\n  nx: [0,%d>\n  ny: [0,%d>\n  nz: [0,%d>\n",__func__ , i,j,k,ecl_grid->nx,ecl_grid->ny,ecl_grid->nz);
    return -1; /* Compiler shut up. */
  }
}


/**
   Converts: active_index -> global_index
*/

int ecl_grid_get_global_index1A(const ecl_grid_type * ecl_grid , int active_index) {
  return ecl_grid->inv_index_map[active_index];
}



/**
   Converts: (i,j,k) -> active_index
   (i,j,k ) are zero offset.
   
   Will return -1 if the cell is not active.
*/

int ecl_grid_get_active_index3(const ecl_grid_type * ecl_grid , int i , int j , int k) {
  int global_index = ecl_grid_get_global_index3(ecl_grid , i,j,k);  /* In range: [0,nx*ny*nz) */
  return ecl_grid_get_active_index1(ecl_grid , global_index);
}


/**
   Converts: global_index -> active_index.
   
   Will return -1 if the cell is not active.
*/

int ecl_grid_get_active_index1(const ecl_grid_type * ecl_grid , int global_index) {
  return ecl_grid->index_map[global_index];
}


/*
  Converts global_index -> (i,j,k)
  
  This function returns C-based zero offset indices. cell_
*/

void ecl_grid_get_ijk1(const ecl_grid_type * grid , int global_index, int *i, int *j , int *k) {
  *k = global_index / (grid->nx * grid->ny); global_index -= (*k) * (grid->nx * grid->ny);
  *j = global_index / grid->nx;              global_index -= (*j) *  grid->nx;
  *i = global_index;
}

/*
  Converts active_index -> (i,j,k)
*/

void ecl_grid_get_ijk1A(const ecl_grid_type *ecl_grid , int active_index , int *i, int * j, int * k) {
  if (active_index >= 0 && active_index < ecl_grid->total_active) {
    int global_index = ecl_grid_get_global_index1A( ecl_grid , active_index );
    ecl_grid_get_ijk1(ecl_grid , global_index , i,j,k);
  } else
    util_abort("%s: error active_index:%d invalid - grid has only:%d active cells. \n",__func__ , active_index , ecl_grid->total_active);
}


/******************************************************************/
/*
  Functions to get the 'true' (i.e. UTM or whatever) position (x,y,z).
*/

/*
  ijk are C-based zero offset.
*/

void ecl_grid_get_pos1(const ecl_grid_type * grid , int global_index , double *xpos , double *ypos , double *zpos) {
  const ecl_cell_type * cell = grid->cells[global_index];
  *xpos = cell->center->x;
  *ypos = cell->center->y;
  *zpos = cell->center->z;
}



void ecl_grid_get_pos3(const ecl_grid_type * grid , int i, int j , int k, double *xpos , double *ypos , double *zpos) {
  const int global_index = ecl_grid_get_global_index__(grid , i , j , k );
  ecl_grid_get_pos1( grid , global_index , xpos , ypos , zpos);
}



void ecl_grid_get_pos1A(const ecl_grid_type * grid , int active_index , double *xpos , double *ypos , double *zpos) {
  const int global_index = ecl_grid_get_global_index1A( grid , active_index );
  ecl_grid_get_pos1( grid , global_index , xpos , ypos , zpos );
}



/**
   Returns the depth of the top surface of the cell. 
*/

double ecl_grid_get_top1(const ecl_grid_type * grid , int global_index) {
  const ecl_cell_type * cell = grid->cells[global_index];
  double depth = 0;
  for (int ij = 0; ij < 4; ij++) 
    depth += cell->corner_list[ij]->z;
  
  return depth * 0.25;
}



double ecl_grid_get_top3(const ecl_grid_type * grid , int i, int j , int k) {
  const int global_index = ecl_grid_get_global_index__(grid , i , j , k );
  return ecl_grid_get_top1( grid , global_index );
}



double ecl_grid_get_top1A(const ecl_grid_type * grid , int active_index) {
  const int global_index = ecl_grid_get_global_index1A(grid , active_index);
  return ecl_grid_get_top1( grid , global_index );
}


/**
   Returns the depth of the bottom surface of the cell. 
*/

double ecl_grid_get_bottom1(const ecl_grid_type * grid , int global_index) {
  const ecl_cell_type * cell = grid->cells[global_index];
  double depth = 0;
  for (int ij = 4; ij < 8; ij++) 
    depth += cell->corner_list[ij]->z;
  
  return depth * 0.25;
}


double ecl_grid_get_bottom3(const ecl_grid_type * grid , int i, int j , int k) {
  const int global_index = ecl_grid_get_global_index__(grid , i , j , k );
  return ecl_grid_get_bottom1( grid , global_index );
}



double ecl_grid_get_bottom1A(const ecl_grid_type * grid , int active_index) {
  const int global_index = ecl_grid_get_global_index1A(grid , active_index);
  return ecl_grid_get_bottom1( grid , global_index );
}




/*****************************************************************/
/* Functions to query whether a cell is active or not.           */

/*
   Global index in [0,...,nx*ny*nz)
*/

bool ecl_grid_cell_active1(const ecl_grid_type * ecl_grid , int global_index) {
  if (ecl_grid->index_map[global_index] >= 0)
    return true;
  else
    return false;
}



bool ecl_grid_cell_active3(const ecl_grid_type * ecl_grid, int i , int j , int k) {
  int global_index = ecl_grid_get_global_index3( ecl_grid , i , j , k);
  return ecl_grid_cell_active1( ecl_grid , global_index );
}


/*****************************************************************/
/* Functions for LGR query/lookup/... */

static void __assert_main_grid(const ecl_grid_type * ecl_grid) {
  if (ecl_grid->grid_nr != 0) 
    util_abort("%s: tried to get LGR grid from another LGR_grid - only main grid can be used as first input \n",__func__);
}


/**
   This functon will return a a ecl_grid instance corresponding to the
   lgr with name lgr_name. The function will fail HARD if no lgr with
   this name is installed under the present main grid; check first
   with ecl_grid_has_lgr() if you are whimp.
   
   Leading/trailing spaces on lgr_name are stripped prior to the hash lookup.
*/


ecl_grid_type * ecl_grid_get_lgr(const ecl_grid_type * main_grid, const char * __lgr_name) {
  __assert_main_grid( main_grid );
  {
    char * lgr_name          = util_alloc_strip_copy( __lgr_name );
    ecl_grid_type * lgr_grid = hash_get(main_grid->LGR_hash , lgr_name);
    free(lgr_name);
    return lgr_grid;
  }
}


/**
   Returns true/false if the main grid has a a lgr with name
   __lgr_name. Leading/trailing spaces are stripped before checking.
*/

bool ecl_grid_has_lgr(const ecl_grid_type * main_grid, const char * __lgr_name) {
  __assert_main_grid( main_grid );
  {
    char * lgr_name          = util_alloc_strip_copy( __lgr_name );
    bool has_lgr             = hash_has_key( main_grid->LGR_hash , lgr_name );
    free(lgr_name);
    return has_lgr;
  }
}


/**
   Return the number of LGR's associated with this main grid
   instance. The main grid is not counted.
*/
int ecl_grid_get_num_lgr(const ecl_grid_type * main_grid ) {
  __assert_main_grid( main_grid );
  return vector_get_size( main_grid->LGR_list ) - 1;  
}

/**
   The lgr_nr has zero offset, not counting the main grid, i.e.

      ecl_grid_iget_lgr( ecl_grid , 0);
   
   will return the first LGR - and fail HARD if there are no LGR's.
*/

ecl_grid_type * ecl_grid_iget_lgr(const ecl_grid_type * main_grid, int lgr_nr) {
  __assert_main_grid( main_grid );
  return vector_iget(  main_grid->LGR_list , lgr_nr + 1);
}


/**
   The following functions will return the LGR subgrid referenced by
   the coordinates given. Observe the following:

   1. The functions will happily return NULL if no LGR is assiciated
      with the cell indicated - in fact that is (currently) the only
      way to query whether a particular cell has a LGR.
      
   2. If a certain cell is refined in several levels this function
      will return a pointer to the first level of refinement. The
      return value can can be used for repeated calls to descend
      deeper into the refinement hierarchy.  
*/


const ecl_grid_type * ecl_grid_get_cell_lgr1(const ecl_grid_type * grid , int global_index ) {
  const ecl_cell_type * cell = grid->cells[global_index];
  return cell->lgr;
}


const ecl_grid_type * ecl_grid_get_cell_lgr3(const ecl_grid_type * grid , int i, int j , int k) {
  const int global_index = ecl_grid_get_global_index__(grid , i , j , k );
  return ecl_grid_get_cell_lgr1( grid , global_index );
}



const ecl_grid_type * ecl_grid_get_cell_lgr1A(const ecl_grid_type * grid , int active_index) {
  const int global_index = ecl_grid_get_global_index1A( grid , active_index );
  return ecl_grid_get_cell_lgr1( grid , global_index );
}


/*****************************************************************/

/** 
    Allocates a stringlist instance with the lookup names of the lgr names in this grid.
*/

stringlist_type * ecl_grid_alloc_lgr_name_list(const ecl_grid_type * ecl_grid) {
  __assert_main_grid( ecl_grid );
  {
    return hash_alloc_stringlist( ecl_grid->LGR_hash );
  }
}



/*****************************************************************/

/**
   This function returns the grid_nr field of the field; this is just
   the occurence number in the grid file. Starting with 0 at the main
   grid, and then increasing consecutively through the lgr sections.

   Observe that there is A MAJOR POTENTIAL for confusion with the
   ecl_grid_iget_lgr() function, the latter does not refer to the main
   grid and returns the first lgr section (which has grid_nr == 1) for
   input argument 0.
*/


int ecl_grid_get_grid_nr( const ecl_grid_type * ecl_grid ) { 
  return ecl_grid->grid_nr; 
}


const char * ecl_grid_get_name( const ecl_grid_type * ecl_grid ) {
  return ecl_grid->name;
}


int ecl_grid_get_global_size( const ecl_grid_type * ecl_grid ) {
  return ecl_grid->nx * ecl_grid->ny * ecl_grid->nz;
}

int ecl_grid_get_active_size( const ecl_grid_type * ecl_grid ) {
  return ecl_grid->total_active;
}


void ecl_grid_summarize(const ecl_grid_type * ecl_grid) {
  int             active_cells , nx,ny,nz;
  ecl_grid_get_dims(ecl_grid , &nx , &ny , &nz , &active_cells);
  printf("	Name ............: %s  \n",ecl_grid->name);
  printf("	Active cells ....: %d \n",active_cells);
  printf("	nx ..............: %d \n",nx);
  printf("	ny ..............: %d \n",ny);
  printf("	nz ..............: %d \n",nz);
  printf("	Volume ..........: %d \n",nx*ny*nz);
  if (ecl_grid->grid_nr == 0) {
    for (int grid_nr=1; grid_nr < vector_get_size( ecl_grid->LGR_list ); grid_nr++) {
      printf("\n");
      ecl_grid_summarize( vector_iget_const( ecl_grid->LGR_list , grid_nr ));
    }
  }
}

/*****************************************************************/
/**
   
   This function is used to translate (with the help of the ecl_grid
   functionality) i,j,k to an index which can be used to look up an
   element in the ecl_kw instance. It is just a minor convenience
   function.

   * If the ecl_kw instance has nx*ny*nz (i,j,k) are translated to a
     global index with ecl_grid_get_global_index3(). This is typically
     the case when the ecl_kw instance represents a petrophysical
     property which is e.g. loaded from a INIT file.

   * If the ecl_kw instance has nactive elements the (i,j,k) indices
     are converted to an active index with
     ecl_grid_get_active_index3(). This is typically the case if the
     ecl_kw instance is a solution vector which has been loaded from a
     restart file. If you ask for an inactive cell the function will
     return 0.

   * If the ecl_kw instance has neither nx*ny*nz nor nactive elements
     the function will fail HARD.

   * The return value is double, irrespective of the type of the
     underlying datatype of the ecl_kw instance - the function will
     fail HARD if the underlying type can not be safely converted to
     double, i.e. if it is not in the set [ecl_float_type ,
     ecl_int_type , ecl_double_type].

   * i,j,k: C-based zero offset grid coordinates.

*/


double ecl_grid_get_property(const ecl_grid_type * ecl_grid , const ecl_kw_type * ecl_kw , int i , int j , int k) {
  ecl_type_enum ecl_type = ecl_kw_get_type( ecl_kw );
  if ((ecl_type == ecl_float_type) || (ecl_type == ecl_int_type) || (ecl_type == ecl_double_type)) {
    int kw_size        = ecl_kw_get_size( ecl_kw );
    int lookup_index   = -1;

    if (kw_size == ecl_grid->nx * ecl_grid->ny * ecl_grid->nz) 
      lookup_index = ecl_grid_get_global_index3(ecl_grid , i , j , k);
    else if (kw_size == ecl_grid->total_active) 
      /* Will be set to -1 if the cell is not active. */ 
      lookup_index = ecl_grid_get_active_index3(ecl_grid , i , j , k);
    else 
      util_abort("%s: incommensurable size ... \n",__func__);

    if (lookup_index >= 0)
      return ecl_kw_iget_as_double( ecl_kw , lookup_index );
    else
      return 0;   /* Tried to lookup an inactive cell. */
  } else {
    util_abort("%s: sorry - can not lookup ECLIPSE type:%s with %s.\n",__func__ , ecl_util_type_name( ecl_type ) , __func__);
    return -1;
  }
}


/*****************************************************************/
/**
   This function will look up all the indices in the grid where the
   region_kw has a certain value (region_value). The ecl_kw instance
   must be loaded beforehand, typically with the functions
   ecl_kw_grdecl_fseek_kw / ecl_kw_fscanf_alloc_grdecl_data.

   The two boolean flags active_only and export_active_index determine
   how active/inactive indieces should be handled:

     active_only: Means that only cells which match the required
        region_value AND are also active are stored. If active_only is
        set to false, ALL cells matching region value are stored in
        index_list.

     export_active_index: if this value is true the the index of the
        cell is in the space of active cells, otherwise it is in terms
        of the global indexing.

   Observe the following about the ecl_kw instance wth region data:

    * It must be of type integer - otherwise we blow up hard.  The
    * size must be the total number of cells (should handle boxes and
      so on ...)

   Observe that there is no way to get ijk from this function, then
   you must call ecl_grid_get_ijk() afterwards. the return value is
   the number of cells found.
*/

int ecl_grid_get_region_cells(const ecl_grid_type * ecl_grid , const ecl_kw_type * region_kw , int region_value , bool active_only, bool export_active_index , int_vector_type * index_list) {
  int cells_found = 0;
  if (ecl_kw_get_size( region_kw ) == ecl_grid->size) {
    if (ecl_kw_get_type( region_kw ) == ecl_int_type) {
      int_vector_reset( index_list );
      const int * region_ptr = ecl_kw_iget_ptr( region_kw , 0);

      /* Layer five hack for Norne: */
      {
        int nx = ecl_grid_get_nx(ecl_grid);
        int ny = ecl_grid_get_ny(ecl_grid);
        int k  = 4; /* == 5 with zero offset*/
        for (int j= 0; j < ny; j++) {
          for (int i = 0; i < nx; i++) {
            int global_index = ecl_grid_get_global_index3( ecl_grid , i , j , k );

            if (region_ptr[global_index] == region_value) {
              if (!active_only || (ecl_grid->index_map[global_index] >= 0)) {
                /* Okay - this index should be included */
                if (export_active_index)
                  int_vector_iset(index_list , cells_found , ecl_grid->index_map[global_index]);
                else
                  int_vector_iset(index_list , cells_found , global_index);
                cells_found++;
              }
            }
          }
        }
      }
      
      /* Proper global code: */
      //{
      //  int global_index;
      //  for (global_index = 0; global_index < ecl_grid->size; global_index++) {
      //     if (region_ptr[global_index] == region_value) {
      //      if (!active_only || (ecl_grid->index_map[global_index] >= 0)) {
      //        /* Okay - this index should be included */
      //        if (export_active_index)
      //          int_vector_iset(index_list , cells_found , ecl_grid->index_map[global_index]);
      //        else
      //          int_vector_iset(index_list , cells_found , global_index);
      //        cells_found++;
      //      }
      //    }
      //  }
      //}
    }  else
      util_abort("%s: type mismatch - regions_kw must be of type integer \n",__func__);

  } else
    util_abort("%s: size mismatch grid has %d cells - region specifier:%d \n",__func__ , ecl_grid->size , ecl_kw_get_size( region_kw ));
  return cells_found;
}
