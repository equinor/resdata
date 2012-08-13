/*
   Copyright (C) 2012  Statoil ASA, Norway. 
   The file 'sum_write' is part of ERT - Ensemble based Reservoir Tool. 
    
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

#include <util.h>
#include <string.h>

#include <ecl_kw.h>
#include <ecl_sum.h>
#include <smspec_node.h>


int main( int argc , char ** argv) {
  time_t start_time = util_make_date( 1,1,2010 );
  int nx = 10;
  int ny = 10;
  int nz = 10;
  

  smspec_node_type * wwct_wellx;
  smspec_node_type * wopr_wellx;

  ecl_sum_type * ecl_sum = ecl_sum_alloc_writer( "/tmp/CASE" , false , true , ":" , start_time , nx , ny , nz );


  ecl_sum_add_var( ecl_sum , "FOPT" , NULL   , 0   , "Barrels" , 99.0 ); 
  ecl_sum_add_var( ecl_sum , "BPR"  , NULL   , 567 , "BARS"    , 0.0  );
  ecl_sum_add_var( ecl_sum , "WWCT" , "OP-1" , 0   , "(1)"     , 0.0  );
  ecl_sum_add_var( ecl_sum , "WOPR" , "OP-1" , 0   , "Barrels" , 0.0  );
  
  wwct_wellx = ecl_sum_add_var( ecl_sum , "WWCT" , NULL , 0 , "(1)"     , 0.0);
  wopr_wellx = ecl_sum_add_var( ecl_sum , "WOPR" , NULL , 0 , "Barrels" , 0.0);
  
  
  {
    int num_dates = 10;
    int num_step = 10;
    double sim_days = 0;

    for (int report_step = 0; report_step < num_dates; report_step++) {
      for (int step = 0; step < num_step; step++) {
        /* Simulate .... */

        sim_days += 10;
        {
          ecl_sum_tstep_type * tstep = ecl_sum_add_tstep( ecl_sum , report_step + 1 , sim_days );
          
          ecl_sum_tstep_set_from_key( tstep  , "WWCT:OP-1" , sim_days / 10);
          if (report_step >= 5)
            ecl_sum_tstep_set_from_node( tstep , wwct_wellx , sim_days );
        }
      }
    }
  }

  
  ecl_sum_update_wgname( ecl_sum , wwct_wellx , "OPX");


  
  ecl_sum_fwrite( ecl_sum );
  ecl_sum_free( ecl_sum );
}
