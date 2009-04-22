/**
  This function implements functionality to load ECLISPE grid files, 
  both .EGRID and .GRID files - in a transparent fashion.

  Observe the following convention:
  
  global_index:  [0 , nx*ny*nz)
  active_index:  [0 , nactive)
*/

#include <stdlib.h>
#include <stdio.h>
#include <util.h>
#include <ecl_kw.h>
#include <ecl_grid.h>
#include <stdbool.h>
#include <ecl_util.h>
#include <double_vector.h>
#include <int_vector.h>
#include <ecl_file.h>

typedef struct ecl_point_struct ecl_point_type;


struct ecl_point_struct {
  double x,y,z;
};


typedef struct ecl_cell_struct ecl_cell_type;


struct ecl_cell_struct {
  bool active;
  int  active_index;
  ecl_point_type center;
  ecl_point_type corner_list[8];
};




struct ecl_grid_struct {
  char                *     filename;
  int nx , ny , nz , size , total_active;
  int                 * index_map;     /* This a list of nx*ny*nz elements, where value -1 means inactive cell .*/
  int                 * inv_index_map; /* This is list of total_active elements - which point back to the index_map. */
  ecl_cell_type      ** cells;

  /*------------------------------: The fields below this line are used for blocking algorithms - and not allocated by default.*/
  int                    block_dim; /* == 2 for maps and 3 for fields. 0 when not in use. */
  int                    block_size;
  int                    last_block_index;
  double_vector_type  ** values;
};



void ecl_point_compare(const ecl_point_type *p1 , const ecl_point_type *p2) {
  if ((abs(p1->x - p2->x) + abs(p1->y - p2->y) + abs(p1->z - p2->z)) > 0.001)
    printf("ERROR");
}


void ecl_cell_compare(const ecl_cell_type * c1 , ecl_cell_type * c2) {
  int i;
  for (i=0; i < 8; i++) 
    ecl_point_compare(&c1->corner_list[i] , &c2->corner_list[i]);
  ecl_point_compare(&c1->center , &c2->center);
}


void ecl_point_printf(const ecl_point_type * p, const char * label, bool newline) {
  if (label != NULL)
    printf("%s:  <%10.3f, %10.3f, %10.3f>",label , p->x, p->y ,p->z);
  else
    printf("<%10.3f, %10.3f, %10.3f>",p->x, p->y ,p->z);

  if (newline)
    printf("\n");
}




/*****************************************************************/

ecl_point_type ecl_point_new(double x, double y , double z) {
  ecl_point_type point;
  point.x = x;
  point.y = y;
  point.z = z;

  return point;
}


void ecl_point_inplace_set(ecl_point_type * point , double x, double y , double z) {
  point->x = x;
  point->y = y;
  point->z = z;
}



void ecl_point_inplace_set_float_ptr(ecl_point_type * point , const float * pos) {
  point->x = pos[0];
  point->y = pos[1];
  point->z = pos[2];
}


void ecl_point_inplace_add(ecl_point_type * point , ecl_point_type add) {
  point->x += add.x;
  point->y += add.y;
  point->z += add.z;
}

void ecl_point_inplace_sub(ecl_point_type * point , ecl_point_type sub) {
  point->x -= sub.x;
  point->y -= sub.y;
  point->z -= sub.z;
}

void ecl_point_inplace_scale(ecl_point_type * point , double scale_factor) {
  point->x *= scale_factor;
  point->y *= scale_factor;
  point->z *= scale_factor;
}


ecl_point_type ecl_point_copy(ecl_point_type src) {
  return ecl_point_new(src.x , src.y , src.z);
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
  return cell;
}


/** 
    Well this is actually quite difficult - the current implementation
    is total and utter crap.
*/




/*
  A x B = [(Ay*Bz - Az*By) , -(Ax*Bz - Az*Bx) , (Ax*By - Bx*Ay)]
*/

/* 
   Computes the signed distance from the point p the plane spanned by
   the two vectors (p1 - p0) x (p2 - p0).
*/


static void __set_normal_vector3d(ecl_point_type * n , ecl_point_type p0 , ecl_point_type p1 , ecl_point_type p2 , bool right_hand) {
  ecl_point_type v1 = p1;
  ecl_point_type v2 = p2;
  
  ecl_point_inplace_sub(&v1 , p0);
  ecl_point_inplace_sub(&v2 , p0);
  n->x =  (v1.y*v2.z - v1.z*v2.y);
  n->y = -(v1.x*v2.z - v1.z*v2.x);
  n->z =  (v1.x*v2.y - v1.y*v2.x);
  if (!right_hand) 
    ecl_point_inplace_scale(n , -1);
}


static double __signed_distance3d(ecl_point_type p0 , ecl_point_type p1 , ecl_point_type p2 , bool right_hand , ecl_point_type p) {
  ecl_point_type n;
  __set_normal_vector3d(&n , p0,p1,p2 ,right_hand);
  ecl_point_inplace_sub(&p  , p0);
  {
    double d = n.x*p.x + n.y*p.y + n.z*p.z;
    return d;
  }
}

static bool __positive_distance3d(ecl_point_type p0 , ecl_point_type p1 , ecl_point_type p2 , bool right_hand , ecl_point_type p) {
  double d = __signed_distance3d(p0 , p1 , p2 , right_hand , p);
  if (d >= 0)
    return true;
  else
    return false;
}



static void __set_normal_vector2d(ecl_point_type * n , ecl_point_type p0 , ecl_point_type p1 , bool right_hand) {
  ecl_point_type v = p1;
  
  ecl_point_inplace_sub(&v , p0);
  n->x = -v.y;
  n->y =  v.x;
  n->z =  0;
  if (!right_hand)
    ecl_point_inplace_scale(n , -1.0);

}


static double __signed_distance2d(ecl_point_type p0 , ecl_point_type p1 , bool right_hand , ecl_point_type p) {
  ecl_point_type n;
  __set_normal_vector2d(&n  ,  p0 , p1 , right_hand);
  ecl_point_inplace_sub(&p  ,  p0);
  {
    double d = n.x*p.x + n.y*p.y;
    return d;
  }
}

static bool __positive_distance2d(ecl_point_type p0 , ecl_point_type p1 , bool right_hand , ecl_point_type p) {
  double d = __signed_distance2d(p0 , p1 , right_hand , p);
  if (d >= 0)
    return true;
  else
    return false;
}



/*

  6---7
  |   |
  4---5



Lower layer:

  2---3
  |   |
  0---1

*/

static bool ecl_cell_contains_3d(const ecl_cell_type * cell , ecl_point_type p) {
  ecl_point_type p0 = cell->corner_list[0];
  ecl_point_type p1 = cell->corner_list[1];
  ecl_point_type p2 = cell->corner_list[2];
  ecl_point_type p3 = cell->corner_list[3];
  ecl_point_type p4 = cell->corner_list[4];
  ecl_point_type p5 = cell->corner_list[5];
  ecl_point_type p6 = cell->corner_list[6];
  //ecl_point_type p7 = cell->corner_list[7];
  bool  contains = false;
  
  if (__positive_distance3d(p0 , p1 , p2 , true , p))        	/* Z1 */
    if (__positive_distance3d(p4 , p5 , p6 , false , p))     	/* Z2 */
      if (__positive_distance3d(p0 , p4 , p2 , true , p))    	/* X1 */
	if (__positive_distance3d(p1 , p5 , p3 , false , p)) 	/* X2 */
	  if (__positive_distance3d(p0 , p4 , p1 , true , p))   /* Y1 */
	    if (__positive_distance3d(p2 , p6 , p3 , false , p))  /* Y2 */
	      contains = true;

  return contains;
}


static void ecl_cell_printf_2dlayer(const ecl_cell_type * cell , bool lower) {
  int offset;
  if (lower)
    offset = 0;
  else
    offset = 4;

  printf("         < %10.3f,%10.3f >              < %10.3f , %10.3f> \n" , cell->corner_list[2+offset].x , cell->corner_list[2+ offset].y , cell->corner_list[3+offset].x , cell->corner_list[3+offset].y);
  printf("                    |                               | \n");
  printf("                    |                               | \n");
  printf("                    |                               | \n");
  printf("                    |                               | \n");
  printf("         < %10.3f,%10.3f >              < %10.3f , %10.3f> \n" , cell->corner_list[0+offset].x , cell->corner_list[0+ offset].y , cell->corner_list[1+offset].x , cell->corner_list[1+offset].y);

}


static bool ecl_cell_contains_2d(const ecl_cell_type * cell , ecl_point_type p) {
  bool contains = false;
  ecl_point_type p0 = cell->corner_list[0];
  ecl_point_type p1 = cell->corner_list[1];
  ecl_point_type p2 = cell->corner_list[2];
  ecl_point_type p3 = cell->corner_list[3];
  
  if (__positive_distance2d(p0 , p2 , false , p))
    if (__positive_distance2d(p0 , p1 , true , p))
      if (__positive_distance2d(p1 , p3 , true , p))
	if (__positive_distance2d(p2 , p3 , false , p))
	  contains = true;
  
  return contains;
}







static int ecl_grid_get_global_index_from_xyz(const ecl_grid_type * grid , double x , double y , double z , int last_index) {
  int global_index;
  ecl_point_type p;
  p.x = x;
  p.y = y;
  p.z = z;
  {
    int index    = 0;
    bool cont    = true;
    global_index = -1;

    do {
      int active_index = ((index + last_index) % grid->block_size);
      bool cell_contains;
      cell_contains = ecl_cell_contains_3d(grid->cells[active_index] , p);

      if (cell_contains) {
	global_index = active_index;
	cont = false;
      }
      index++;
      if (index == grid->block_size)
	cont = false;
    } while (cont);
  }
  return global_index;
}



static int ecl_grid_get_global_index_from_xy(const ecl_grid_type * grid , double x , double y , int last_index) {
  int global_index;
  ecl_point_type p;
  p.x = x;
  p.y = y;
  p.z = -1;
  {
    int index    = 0;
    bool cont    = true;
    global_index = -1;

    do {
      int active_index = ((index + last_index) % grid->block_size);
      bool cell_contains;
      cell_contains = ecl_cell_contains_2d(grid->cells[active_index] , p);
      
      if (cell_contains) {
	global_index = active_index;
	cont = false;
      } 
      index++;
      if (index == grid->block_size)
	cont = false;
    } while (cont);
  }
  return global_index;
}
      
  

void ecl_cell_free(ecl_cell_type * cell) {
  free(cell);
}


static ecl_grid_type * ecl_grid_alloc_empty(int nx , int ny , int nz) {
  ecl_grid_type * grid = util_malloc(sizeof * grid , __func__);
  grid->nx = nx;
  grid->ny = ny;
  grid->nz = nz;
  grid->size = nx*ny*nz;

  grid->inv_index_map = NULL;
  grid->index_map     = NULL;
  grid->cells         = util_malloc(nx*ny*nz * sizeof * grid->cells , __func__);
  {
    int i;
    for (i=0; i < grid->size; i++)
      grid->cells[i] = ecl_cell_alloc();
  }
  grid->block_dim      = 0;
  grid->values         = NULL;
  return grid;
}


/*
  This function wants C-based zero offset on i,j,k - ohh what a fuxxx mess.
*/

static inline int ecl_grid_get_global_index__(const ecl_grid_type * ecl_grid , int i , int j , int k) {
  return i + j * ecl_grid->nx + k * ecl_grid->nx * ecl_grid->ny;
}

  

/**
   i,j,k have zero based offset.
*/
int ecl_grid_get_global_index(const ecl_grid_type * ecl_grid , int i , int j , int k) {
  if (ecl_grid_ijk_valid(ecl_grid , i , j , k))
    return ecl_grid_get_global_index__(ecl_grid , i , j , k);
  else {
    util_abort("%s: i,j,k = (%d,%d,%d) is invalid:\n\n  nx: [0,%d>\n  ny: [0,%d>\n  nz: [0,%d>\n",__func__ , i,j,k,ecl_grid->nx,ecl_grid->ny,ecl_grid->nz);
    return -1; /* Compiler shut up. */
  }
}


/**
   Return the active cell index (of the active cells) of the cell
   i,j,k. (with zero based offset).

   Will return -1 if the cell is not active.
*/

int ecl_grid_get_active_index(const ecl_grid_type * ecl_grid , int i , int j , int k) {
  int global_index = ecl_grid_get_global_index(ecl_grid , i,j,k);  /* In range: [0,nx*ny*nz) */
  return ecl_grid->index_map[global_index];
}


bool ecl_grid_ijk_active(const ecl_grid_type * ecl_grid, int i , int j , int k) {
  int active_index = ecl_grid_get_active_index( ecl_grid , i , j , k);
  if (active_index >= 0)
    return true;
  else
    return false;
}


int ecl_grid_get_active_index_from_global(const ecl_grid_type * ecl_grid , int global_index) {
  return ecl_grid->index_map[global_index];
}

int ecl_grid_get_global_index_from_active(const ecl_grid_type * ecl_grid , int active_index) {
  return ecl_grid->inv_index_map[active_index];
}


int ecl_grid_get_size( const ecl_grid_type * grid) {
  return grid->size;
}


/* 
   This function returns C-based zero offset indices. cell_
*/
void ecl_grid_get_ijk(const ecl_grid_type * grid , int global_index, int *i, int *j , int *k) {
  *k = global_index / (grid->nx * grid->ny); global_index -= (*k) * (grid->nx * grid->ny);
  *j = global_index / grid->nx;              global_index -= (*j) *  grid->nx;
  *i = global_index;
}


void ecl_grid_get_ijk_from_active_index(const ecl_grid_type *ecl_grid , int active_index , int *i, int * j, int * k) {
  if (active_index >= 0 && active_index < ecl_grid->total_active) {
    int global_index = ecl_grid_get_global_index_from_active( ecl_grid , active_index );
    ecl_grid_get_ijk(ecl_grid , global_index , i,j,k);
  } else
    util_abort("%s: error active_index:%d invalid - grid has only:%d active cells. \n",__func__ , active_index , ecl_grid->total_active);
}



static void ecl_grid_set_center(ecl_grid_type * ecl_grid) {
  int c , i;
  for (i=0; i < ecl_grid->size; i++) {
    ecl_cell_type * cell = ecl_grid->cells[i];
    ecl_point_inplace_set(&cell->center , 0,0,0);
    for (c = 0; c < 8; c++) 
      ecl_point_inplace_add(&cell->center , cell->corner_list[c]);
    ecl_point_inplace_scale(&cell->center , 1.0 / 8.0);
  }
}



static void ecl_grid_set_cell_EGRID(ecl_grid_type * ecl_grid , int i, int j , int k , double x[4][2] , double y[4][2] , double z[4][2] , const int * actnum) {

  const int global_index   = ecl_grid_get_global_index__(ecl_grid , i , j  , k );
  ecl_cell_type * cell     = ecl_grid->cells[global_index];
  int ip , iz;
  
  for (iz = 0; iz < 2; iz++) {
    for (ip = 0; ip < 4; ip++) {
      int c = ip + iz * 4;
      ecl_point_inplace_set(&cell->corner_list[c] , x[ip][iz] , y[ip][iz] , z[ip][iz]);
    }
  }
  cell->active       = actnum[global_index];
}


static void ecl_grid_set_cell_GRID(ecl_grid_type * ecl_grid , const ecl_kw_type * coords_kw , const ecl_kw_type * corners_kw) {
  const int   * coords  = ecl_kw_get_int_ptr(coords_kw);  
  const float * corners = ecl_kw_get_float_ptr(corners_kw);   
  const int i  = coords[0]; /* ECLIPSE 1 offset */
  const int j  = coords[1];
  const int k  = coords[2];
  const int global_index   = ecl_grid_get_global_index__(ecl_grid , i - 1, j - 1 , k - 1);
  ecl_cell_type * cell     = ecl_grid->cells[global_index];
  int c;

  /* The coords keyword can optionally contain 4,5 or 7 elements: 

     	coords[0..2] = i,j,k
     	coords[3]    = global_cell number (not used here)
     	----
     	coords[4]    = active
     	coords[5..6] = lgr stuff

     If coords[4] is not present it is assumed that the cell is active.
  */
     
  
  if (ecl_kw_get_size(coords_kw) >= 5)
    cell->active  = (coords[4] == 1) ? true : false;
  else
    cell->active = true;
  
  for (c = 0; c < 8; c++) 
    ecl_point_inplace_set_float_ptr(&cell->corner_list[c] , (const float *) &corners[c*3]);
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



static void ecl_grid_pillar_cross_planes(const ecl_point_type * pillar , const double *z , double *x , double *y) {
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


/*
  2---3
  |   |
  0---1
*/

ecl_grid_type * ecl_grid_alloc_GRDECL(int nx , int ny , int nz , const float * zcorn , const float * coord , const int * actnum) {
  int i,j,k;
  ecl_grid_type * ecl_grid = ecl_grid_alloc_empty(nx,ny,nz);
  ecl_point_type pillars[4][2];
  
  for (j=0; j < ny; j++) {
    for (i=0; i < nx; i++) {
      int pillar_index[4];
      int ip;
      pillar_index[0] = 6 * ( j      * (nx + 1) + i    );
      pillar_index[1] = 6 * ( j      * (nx + 1) + i + 1);
      pillar_index[2] = 6 * ((j + 1) * (nx + 1) + i    );
      pillar_index[3] = 6 * ((j + 1) * (nx + 1) + i + 1);
      
      for (ip = 0; ip < 4; ip++) {
	ecl_point_inplace_set_float_ptr(&pillars[ip][0] , (const float *) &coord[pillar_index[ip]    ]);
	ecl_point_inplace_set_float_ptr(&pillars[ip][1] , (const float *) &coord[pillar_index[ip] + 3]);
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
  
  ecl_grid_set_center(ecl_grid);
  ecl_grid_set_active_index(ecl_grid);
  return ecl_grid;
}



static ecl_grid_type * ecl_grid_alloc_EGRID(const char * grid_file , bool endian_flip) {
  ecl_grid_type * ecl_grid;
  ecl_file_enum   file_type;
  bool            fmt_file;
  ecl_util_get_file_type(grid_file , &file_type , &fmt_file , NULL);
  if (file_type != ecl_egrid_file) 
    util_abort("%s: %s wrong file type - expected .EGRID file - aborting \n",__func__ , grid_file);
  
  {
    ecl_file_type * ecl_file  = ecl_file_fread_alloc( grid_file , endian_flip );
    ecl_kw_type * gridhead_kw = ecl_file_iget_named_kw( ecl_file , "GRIDHEAD" , 0);
    int gtype, nx,ny,nz;
    
    gtype = ecl_kw_iget_int(gridhead_kw , 0);
    nx 	  = ecl_kw_iget_int(gridhead_kw , 1);
    ny 	  = ecl_kw_iget_int(gridhead_kw , 2);
    nz 	  = ecl_kw_iget_int(gridhead_kw , 3);
    if (gtype != 1) 
      util_abort("%s: gtype:%d fatal error when loading:%s - must have corner point grid - aborting\n",__func__ , gtype , grid_file);
    {
      ecl_kw_type * zcorn_kw    = ecl_file_iget_named_kw( ecl_file , "ZCORN"   	, 0);
      ecl_kw_type * coord_kw    = ecl_file_iget_named_kw( ecl_file , "COORD"   	, 0);
      ecl_kw_type * actnum_kw   = ecl_file_iget_named_kw( ecl_file , "ACTNUM"  	, 0);
      
      ecl_grid = ecl_grid_alloc_GRDECL(nx , ny , nz , ecl_kw_get_float_ptr(zcorn_kw) , ecl_kw_get_float_ptr(coord_kw) , ecl_kw_get_int_ptr(actnum_kw));
    }
    ecl_file_free( ecl_file );
  }
  return ecl_grid;
}



const int * ecl_grid_get_index_map_ref(const ecl_grid_type * grid) {
  return grid->index_map;
}



/**
   This function allocates a copy of the index_map and returns it.
*/


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




static ecl_grid_type * ecl_grid_alloc_GRID(const char * grid_file, bool endian_flip) {
  
  ecl_file_enum   file_type;
  int             nx,ny,nz,index;
  ecl_grid_type * grid;
  ecl_util_get_file_type(grid_file , &file_type , NULL , NULL);  
  if (file_type != ecl_grid_file) 
    util_abort("%s: %s wrong file type - expected .GRID file - aborting \n",__func__ , grid_file);

  {
    ecl_file_type * ecl_file = ecl_file_fread_alloc( grid_file , endian_flip );
    
    {
      ecl_kw_type * dimens_kw = ecl_file_iget_named_kw( ecl_file , "DIMENS" , 0);
      nx = ecl_kw_iget_int(dimens_kw , 0);
      ny = ecl_kw_iget_int(dimens_kw , 1);
      nz = ecl_kw_iget_int(dimens_kw , 2);
      grid = ecl_grid_alloc_empty(nx , ny , nz);
    }
    
    for (index = 0; index < ecl_file_get_num_named_kw( ecl_file , "COORDS"); index++) {
      ecl_kw_type * coords_kw  = ecl_file_iget_named_kw(ecl_file , "COORDS"  , index);
      ecl_kw_type * corners_kw = ecl_file_iget_named_kw(ecl_file , "CORNERS" , index);
      ecl_grid_set_cell_GRID(grid , coords_kw , corners_kw);
    }
    
    ecl_file_free( ecl_file );
  }
  
  ecl_grid_set_center(grid);
  ecl_grid_set_active_index(grid);
  
  return grid;
}


void ecl_grid_get_dims(const ecl_grid_type * grid , int *nx , int * ny , int * nz , int * active_size) {
  if (nx != NULL) *nx 	       		= grid->nx;
  if (ny != NULL) *ny 	       		= grid->ny;
  if (nz != NULL) *nz 	       		= grid->nz;
  if (active_size != NULL) *active_size = grid->total_active;
}


/** 
    Input is assumed to be C-based zero offset.
*/

inline bool ecl_grid_ijk_valid(const ecl_grid_type * grid , int i , int j , int k) {
  bool OK = false;

  if (i >= 0 && i < grid->nx)
    if (j >= 0 && j < grid->ny)
      if (k >= 0 && k < grid->nz)
	OK = true;
  
  return OK;
}



/**
   This function will allocate a ecl_grid instance. As input it takes
   a filename, which can be both a GRID file and an EGRID file (both
   formatted and unformatted). 
*/

ecl_grid_type * ecl_grid_alloc(const char * grid_file , bool endian_flip) {
  ecl_file_enum    file_type;
  bool             fmt_file;
  ecl_grid_type  * ecl_grid = NULL;
  
  ecl_util_get_file_type(grid_file , &file_type , &fmt_file , NULL);
  if (file_type == ecl_grid_file)
    ecl_grid = ecl_grid_alloc_GRID(grid_file , endian_flip);
  else if (file_type == ecl_egrid_file)
    ecl_grid = ecl_grid_alloc_EGRID(grid_file , endian_flip);
  else 
    util_abort("%s must have .GRID or .EGRID file - %s not recognized \n", __func__ , grid_file);

  ecl_grid->filename = util_alloc_string_copy( grid_file );
  ecl_grid_alloc_index_map(ecl_grid);
  return ecl_grid;
}


const char * ecl_grid_get_filename( const ecl_grid_type * ecl_grid ) {
  return ecl_grid->filename;
}





void ecl_grid_compare(const ecl_grid_type * g1 , const ecl_grid_type * g2) {
  int i;
  for (i = 0; i < g1->size; i++) {
    ecl_cell_type *c1 = g1->cells[i];
    ecl_cell_type *c2 = g2->cells[i];
    ecl_cell_compare(c1 , c2);
  }
  printf("Compare OK \n");
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
    grid->values[index] = double_vector_alloc(4 , 0.0);
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
    int global_index = ecl_grid_get_global_index_from_xyz( grid , x , y , z , grid->last_block_index);
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
    int global_index = ecl_grid_get_global_index_from_xy( grid , x , y , grid->last_block_index);
    if (global_index >= 0) {
      double_vector_append( grid->values[global_index] , value);
      grid->last_block_index = global_index;
      return true;
    } else
      return false;
  }
}



double ecl_grid_block_eval2d(ecl_grid_type * grid , int i, int j , block_function_ftype * blockf ) {
  int global_index = ecl_grid_get_global_index(grid , i,j,0);
  return blockf( grid->values[global_index]);
}


double ecl_grid_block_eval3d(ecl_grid_type * grid , int i, int j , int k ,block_function_ftype * blockf ) {
  int global_index = ecl_grid_get_global_index(grid , i,j,k);
  return blockf( grid->values[global_index]);
}

int ecl_grid_get_block_count2d(const ecl_grid_type * grid , int i , int j) {
  int global_index = ecl_grid_get_global_index(grid , i,j,0);
  return double_vector_size( grid->values[global_index]);
}


int ecl_grid_get_block_count3d(const ecl_grid_type * grid , int i , int j, int k) {
  int global_index = ecl_grid_get_global_index(grid , i,j,k);
  return double_vector_size( grid->values[global_index]);
}


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
  free( grid->filename );
  free( grid );
}




void ecl_grid_set_box_active_list(const ecl_grid_type * grid , const ecl_box_type * box , int * active_index_list) {
  int i1,i2,j1,j2,k1,k2;
  int i,j,k;
  int active_count = 0;
  ecl_box_set_limits(box , &i1 , &i2 , &j1 , &j2 , &k1 , &k2); 

  for (k = k1; k <= k2; k++) {
    for (j= j1; j <= j2; j++) {
      for (i=i1; i <= i2; i++) {
	const int global_index = ecl_grid_get_global_index__(grid , i , j , k );
	ecl_cell_type * cell   = grid->cells[global_index];
	if (cell->active) {
	  active_index_list[active_count] = cell->active_index;
	  active_count++;
	}
      }
    }
  }
}




int ecl_grid_count_box_active(const ecl_grid_type * grid , const ecl_box_type * box) {
  int i1,i2,j1,j2,k1,k2;
  int i,j,k;
  int active_count = 0;
  ecl_box_set_limits(box , &i1 , &i2 , &j1 , &j2 , &k1 , &k2); 

  for (k = k1; k <= k2; k++) {
    for (j= j1; j <= j2; j++) {
      for (i=i1; i <= i2; i++) {
	const int global_index   = ecl_grid_get_global_index__(grid , i , j , k );
	ecl_cell_type * cell   = grid->cells[global_index];
	if (cell->active)
	  active_count++;
      }
    }
  }
  
  return active_count;
}



void ecl_grid_get_distance(const ecl_grid_type * grid , int global_index1, int global_index2 , double *dx , double *dy , double *dz) {
  const ecl_cell_type * cell1 = grid->cells[global_index1];
  const ecl_cell_type * cell2 = grid->cells[global_index2];
  
  *dx = cell1->center.x - cell2->center.x;
  *dy = cell1->center.y - cell2->center.y;
  *dz = cell1->center.z - cell2->center.z;
  
}


/*
  ijk are C-based zero offset. 
*/
void ecl_grid_get_pos(const ecl_grid_type * grid , int i, int j , int k, double *xpos , double *ypos , double *zpos) {
  const int global_index     = ecl_grid_get_global_index__(grid , i , j , k );
  const ecl_cell_type * cell = grid->cells[global_index];

  *xpos = cell->center.x;
  *ypos = cell->center.y;
  *zpos = cell->center.z;
}



void ecl_grid_summarize(const ecl_grid_type * ecl_grid) {
  int             active_cells , nx,ny,nz;
  ecl_grid_get_dims(ecl_grid , &nx , &ny , &nz , &active_cells);
  printf("	Active cells ....: %d \n",active_cells);
  printf("	nx ..............: %d \n",nx);
  printf("	ny ..............: %d \n",ny);
  printf("	nz ..............: %d \n",nz);
  printf("	Volume ..........: %d \n",nx*ny*nz);
}



/*****************************************************************/
/**
   This function will look up all the indices in the grid where the
   region_kw has a certan value (region_value). The ecl_kw instance
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
        of the gloabl indexing.
  
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
      int global_index;
      for (global_index = 0; global_index < ecl_grid->size; global_index++) {
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
    }  else
      util_abort("%s: type mismatch - regions_kw must be of type integer \n",__func__);
    
  } else
    util_abort("%s: size mismatch grid has %d cells - region specifier:%d \n",__func__ , ecl_grid->size , ecl_kw_get_size( region_kw ));
  return cells_found;
}
