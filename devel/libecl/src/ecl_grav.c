#include <stdlib.h>
#include <math.h>
#include <stdbool.h>
#include <ecl_kw.h>
#include <ecl_file.h>
#include <ecl_grid.h>
#include <ecl_grav.h>
#include <util.h>



double ecl_grav_phase_deltag( double utm_x ,
                              double utm_y , 
                              double tvd,
                              const ecl_grid_type * grid,
                              const ecl_kw_type   * rho_kw,
                              const ecl_kw_type   * aquifern_kw,
                              const ecl_kw_type   * sat1_kw,
                              const ecl_kw_type   * porv1_kw,
                              const ecl_kw_type   * sat2_kw,
                              const ecl_kw_type   * porv2_kw) {
  double deltag = 0;
  float * aquifern;
  const float * rho     = ecl_kw_get_float_ptr( rho_kw );
  const float * sat1    = ecl_kw_get_float_ptr( sat1_kw );
  const float * porv1   = ecl_kw_get_float_ptr( porv1_kw );
  const float * sat2    = ecl_kw_get_float_ptr( sat2_kw );
  const float * porv2   = ecl_kw_get_float_ptr( porv2_kw );
  
  if (aquifern_kw != NULL)
    aquifern = ecl_kw_get_float_ptr( aquifern_kw );
  else {
    aquifern = util_malloc( ecl_grid_get_active_size( grid ) * sizeof * aquifern , __func__ );
    for (int i=0; i < ecl_grid_get_active_size( grid ); i++)
      aquifern[i] = 0;
  }
  
  {
    int active_index;
    for (active_index = 0; active_index < ecl_grid_get_active_size( grid ); active_index++) {
      if (aquifern[ active_index ] == 0) {
        /* Ensure that this cell is not a numerical aquifer. */
        double  mas1 , mas2;
        double  xpos , ypos , zpos;

        mas1 = porv1[active_index] * sat1[active_index];
        mas2 = porv2[active_index] * sat2[active_index];
        
        ecl_grid_get_xyz1A(grid , active_index , &xpos , &ypos , &zpos);
        {
          double dist_x   = xpos - utm_x;
          double dist_y   = ypos - utm_y;
          double dist_z   = zpos - tvd;
          double dist_sq  = dist_x*dist_x + dist_y*dist_y + dist_z*dist_z;
          
          if(dist_sq == 0)
            exit(1);
          
          deltag += 6.67E-3*(mas2 - mas1)*rho[active_index]*dist_z/pow(dist_sq, 1.5);
        }
      }
    }
  }

  if (aquifern_kw == NULL)
    free( aquifern );

  return deltag;
}

