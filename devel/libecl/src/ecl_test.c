#include <stdlib.h>
#include <ecl_kw.h>
#include <fortio.h>
#include <util.h>
#include <string.h>
#include <ecl_util.h>
#include <ecl_sum.h>
#include <hash.h>
#include <stdbool.h>
#include <ecl_rft_file.h>
#include <ecl_grid.h>
#include <ecl_smspec.h>
#include <ecl_sum_data.h>
#include <ecl_file.h>


int main (int argc, char **argv)
{
//  char * grid_file = argv[1];
//  char * init_file = argv[2];
//
//  ecl_grid_type * grid = ecl_grid_alloc( grid_file );
//  ecl_file_type * init = ecl_file_fread_alloc( init_file );
//  ecl_kw_type * PORV   = ecl_file_iget_named_kw( init , "PORV" , 0);
//  ecl_kw_type * NTG    = ecl_file_iget_named_kw( init , "NTG"  , 0);
//  ecl_kw_type * PORO   = ecl_file_iget_named_kw( init , "PORO" , 0);
//  ecl_kw_type * DX     = ecl_file_iget_named_kw( init , "DX"   , 0);
//  ecl_kw_type * DY     = ecl_file_iget_named_kw( init , "DY"   , 0);
//  ecl_kw_type * DZ     = ecl_file_iget_named_kw( init , "DZ"   , 0);
//  
//  {
//    int i,j,k;
//    
//    double dx,dy,dz,poro,porv,ntg,volume;
//
//    i = 15;
//    j = 13;
//    k = 7;
//
//    dx     = ecl_grid_get_property( grid , DX , i , j , k);
//    dy     = ecl_grid_get_property( grid , DY , i , j , k);
//    dz     = ecl_grid_get_property( grid , DZ , i , j , k);
//    poro   = ecl_grid_get_property( grid , PORO , i , j , k);
//    porv   = ecl_grid_get_property( grid , PORV , i , j , k);
//    ntg    = ecl_grid_get_property( grid , NTG  , i , j , k);
//    volume = ecl_grid_get_cell_volume3( grid , i , j , k);
//    
//    printf("i:%2d  j:%2d  k:%2d  volume:%g   dx*dy*dz:%g  dx*dy*dz * ntg * poro:%g  porv:%g \n",i,j,k,volume*poro*ntg , dx*dy*dz , dx*dy*dz*ntg*poro , porv);
//  }
//  ecl_grid_free( grid );
//  ecl_file_free( init );
}
