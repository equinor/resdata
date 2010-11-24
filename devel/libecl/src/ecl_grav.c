#include <stdlib.h>
#include <math.h>
#include <stdbool.h>
#include <ecl_kw.h>
#include <ecl_file.h>
#include <ecl_grid.h>
#include <ecl_grav.h>
#include <util.h>



double ecl_grav_phase_test_deltag( double utm_x ,
                                   double utm_y , 
                                   double tvd,
                                   const ecl_grid_type * grid,
                                   const ecl_kw_type   * aquifern_kw,
                                   const ecl_kw_type   * sat_kw,
                                   const ecl_kw_type   * rho_kw,
                                   const ecl_kw_type   * porv_kw) {
  
  
  printf("Aquifer_kw:%p\n",aquifern_kw);
  printf("rho_kw :%p \n",rho_kw );
  printf("sat_kw :%p \n",sat_kw );
  printf("porv_kw :%p \n",porv_kw );
  printf("-----------------------------------------------------------------\n");
  return -1;
}



double ecl_grav_phase_deltag( double utm_x ,
                              double utm_y , 
                              double tvd,
                              const ecl_grid_type * grid,
                              const ecl_kw_type   * aquifern_kw,
                              const ecl_kw_type   * sat1_kw,
                              const ecl_kw_type   * rho1_kw,
                              const ecl_kw_type   * porv1_kw,
                              const ecl_kw_type   * sat2_kw,
                              const ecl_kw_type   * rho2_kw,
                              const ecl_kw_type   * porv2_kw) {

  double deltag = 0;
  float * aquifern;
  
  //printf("Aquifer_kw:%p\n",aquifern_kw);
  //printf("rho1_kw :%p \n",rho1_kw );
  //printf("rho2_kw :%p \n",rho2_kw );
  //printf("sat1_kw :%p \n",sat1_kw );
  //printf("sat2_kw :%p \n",sat2_kw );
  //printf("porv1_kw :%p \n",porv1_kw );
  //printf("porv2_kw :%p \n",porv2_kw );
  
  const float * rho1    = ecl_kw_get_float_ptr( rho1_kw );
  const float * rho2    = ecl_kw_get_float_ptr( rho2_kw );
  const float * sat1    = ecl_kw_get_float_ptr( sat1_kw );
  const float * sat2    = ecl_kw_get_float_ptr( sat2_kw );
  const float * porv1   = ecl_kw_get_float_ptr( porv1_kw );
  const float * porv2   = ecl_kw_get_float_ptr( porv2_kw );

  

  if (aquifern_kw != NULL)
    aquifern = ecl_kw_get_int_ptr( aquifern_kw );
  else  {
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

        mas1 = rho1[ active_index ] * porv1[active_index] * sat1[active_index];
        mas2 = rho2[ active_index ] * porv2[active_index] * sat2[active_index];
        
        ecl_grid_get_xyz1A(grid , active_index , &xpos , &ypos , &zpos);
        {
          double dist_x   = xpos - utm_x;
          double dist_y   = ypos - utm_y;
          double dist_z   = zpos - tvd;
          double dist_sq  = dist_x*dist_x + dist_y*dist_y + dist_z*dist_z;
          
          if(dist_sq == 0)
            exit(1);
          
          deltag += 6.67E-3*(mas2 - mas1) * dist_z/pow(dist_sq, 1.5);
        }
      }
    }
  }

  if (aquifern_kw == NULL)
    free( aquifern );

  return deltag;
}

