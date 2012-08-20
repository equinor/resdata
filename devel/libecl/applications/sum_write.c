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


/*
  The ECLIPSE summary data is stored in two different files:

   - A header file which contains metadata of the stored data; this
     header file has extension SMSPEC.

   - The actual data is stored as blocks of data where each block
     contains all the variables for one timestep (called MINISTEP in
     ECLIPSE lingo). The data can be stored in a unified summary file
     with all the data in file, or alternatively several separate
     files - one for each report step.

     In the case of a unified file the extension is UNSMRY whereas the
     multiple summary files haver extension Snnnn.

  The format is quite primitive, and very much influenced by Fortran77
  arrays which are fixed-size-large-enough. The header file is
  (mainly) based on three different ecl_kw vectors which together
  completely specify the variables. The example below consists of the
  following:

    - Variables: Field Oil Production Total (FOPT), Group Oil
      Production Rate (GOPR) in group P-North, the Block Pressure
      (BPR) in cell 5423 and the Well Gas Production Rate (WGPR) in
      the well GasW.

    - A total of two report steps (dates), where the first consist of
      two timesteps and the latter of three timesteps.



             CASE.SMSPEC                      CASE.S0001                         CASE.S0002
   ------------------------------       --------------------------       --------------------------------------- 
   | KEYWORD |  WGNAMES  | NUMS |       | MINISTEP 1|  MINISTEP 2|       | MINISTEP 3|  MINISTEP 4|  MINISTEP 5|
   +----------------------------+       +------------------------+       +-------------------------------------+
   | TIME    |           |      |  -->  |           |            |  -->  |           |            |            |
   | FOPT    |           |      |  -->  |           |            |  -->  |           |            |            |
   | GOPR    | P-North   |      |  -->  |           |            |  -->  |           |            |            |
   | BPR     |           | 5423 |  -->  |           |            |  -->  |           |            |            |
   | WGPR    | GasW      |      |  -->  |           |            |  -->  |           |            |            |
   +----------------------------+       +------------------------+       +-------------------------------------+

                                        |<---                 --  CASE.UNSMRY --                           --->|

  Observe the following points:

   1. The variable name supplied in the KEYWORD array is significant;
      ECLIPSE will determine the type of variable by looking at the
      first letter of the keyword: "W" for well variables, "F" for
      field variables and so on. This is further documented in the
      ecl_smspec_identify_var_type() function.

   2. The KEYWORD vector in the SMSPEC file is relevant for all the
      entries, whereas the WGNAMES and NUMS entries should be
      considered as further qualifiers which apply selectively
      depending on the variable type - for instance both the WGNAMES
      and NUMS vectors are ignored in the case of field variable like
      FOPT.

   3. The actual data is organised in blocks of data, one continous
      block for each (simulator) timestep. These blocks are collected
      in report steps; when stored the data can be either lumped
      together in a unified file, or split in many different files.

   4. Each block of data is exactly as large the header. This implies
      that: 

        - The number of elements in the summary file must be known up
          front; it is not possible to add/remove variables during
          simulation.
        - All timesteps are equally large.

      Due to this fixed size nature an important element in the
      ecl_sum implementation is "the index of a variable" - i.e. the
      "FOPT" variable in the example above will be found at index 1 of
      every datablock.

   5. The header contains a "TIME" variable; to some extent this looks
      like just any other variable, but it must be present in the
      SMSPEC header. In the example above the first element in every
      data block is the current time (in days) for that datablock.
      

  Due to the fixed size nature of the summary file format described in
  point 4 above the library is somewhat cumbersome and unflexible to
  use. Very briefly the usage pattern will be like:

    1. Create a ecl_sum instance and add variables to with the
       ecl_sum_add_var() function.

    2. Use your simulator to step forward in time, and add timesteps
       with ecl_sum_add_tstep().
  
   Now - the important thing is that steps 1 and 2 two can not be
   interchanged, that will lead to crash and burn.  
*/
  


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
