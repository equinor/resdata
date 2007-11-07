#include <stdlib.h>
#include <ecl_kw.h>
#include <fortio.h>
#include <util.h>
#include <string.h>
#include <ecl_util.h>
#include <ecl_sum.h>
#include <hash.h>
#include <stdbool.h>
#include <ecl_rft_vector.h>


int main(int argc, char ** argv) {
  ecl_rft_vector_type * rft_vector = ecl_rft_vector_alloc("/d/proj/bg/enkf/EnKF_OE/Refcase/ECLIPSE_030807.RFT", true);
  {
    int wells , i;
    char ** well_list = ecl_rft_vector_alloc_well_list(rft_vector , &wells);
    for (i=0; i < wells; i++)
      printf("well[%2d] = %s \n",i,well_list[i]);
  }
  ecl_rft_vector_fprintf_rft_obs(rft_vector , "E4" 	 , "/d/proj/bg/enkf/EnKF_OE/Static/rft/E4"       , "/d/proj/bg/enkf/EnKF_OE/Run3/Observations/0007/RFT" , 1.0);
  ecl_rft_vector_fprintf_rft_obs(rft_vector , "E6" 	 , "/d/proj/bg/enkf/EnKF_OE/Static/rft/E6"       , "/d/proj/bg/enkf/EnKF_OE/Run3/Observations/0020/RFT" , 1.0);  /* E6: To ganger */
  /*ecl_rft_vector_fprintf_rft_obs(rft_vector , "E14_RFT"  , "/d/proj/bg/enkf/EnKF_OE/Static/rft/E14_RFT"  , "/d/proj/bg/enkf/EnKF_OE/Run3/Observations/0022/RFT" , 1.0);*/
  ecl_rft_vector_fprintf_rft_obs(rft_vector , "E6" 	 , "/d/proj/bg/enkf/EnKF_OE/Static/rft/E6"       , "/d/proj/bg/enkf/EnKF_OE/Run3/Observations/0027/RFT" , 1.0);
  ecl_rft_vector_fprintf_rft_obs(rft_vector , "E1" 	 , "/d/proj/bg/enkf/EnKF_OE/Static/rft/E1"       , "/d/proj/bg/enkf/EnKF_OE/Run3/Observations/0030/RFT" , 1.0);
  /*ecl_rft_vector_fprintf_rft_obs(rft_vector , "E8_RFT" 	 , "/d/proj/bg/enkf/EnKF_OE/Static/rft/E8_rft"   , "/d/proj/bg/enkf/EnKF_OE/Run3/Observations/0039/RFT" , 1.0);
    ecl_rft_vector_fprintf_rft_obs(rft_vector , "E9_RFT" 	 , "/d/proj/bg/enkf/EnKF_OE/Static/rft/E9_rft"   , "/d/proj/bg/enkf/EnKF_OE/Run3/Observations/0042/RFT" , 1.0);  E9_RFT to ganger */
  /*ecl_rft_vector_fprintf_rft_obs(rft_vector , "E12_RFT"  , "/d/proj/bg/enkf/EnKF_OE/Static/rft/E12_rft"  , "/d/proj/bg/enkf/EnKF_OE/Run3/Observations/0048/RFT" , 1.0);*/
  /*ecl_rft_vector_fprintf_rft_obs(rft_vector , "E9_RFT" 	 , "/d/proj/bg/enkf/EnKF_OE/Static/rft/E9_rft"   , "/d/proj/bg/enkf/EnKF_OE/Run3/Observations/0051/RFT" , 1.0);*/
  /*ecl_rft_vector_fprintf_rft_obs(rft_vector , "E9A_RFT"  , "/d/proj/bg/enkf/EnKF_OE/Static/rft/E9A_rft"  , "/d/proj/bg/enkf/EnKF_OE/Run3/Observations/0055/RFT" , 1.0);*/
  /*ecl_rft_vector_fprintf_rft_obs(rft_vector , "E12W"     , "/d/proj/bg/enkf/EnKF_OE/Static/rft/E12W"     , "/d/proj/bg/enkf/EnKF_OE/Run3/Observations/0057/RFT" , 1.0);*/
  /*ecl_rft_vector_fprintf_rft_obs(rft_vector , "E8" 	 , "/d/proj/bg/enkf/EnKF_OE/Static/rft/E8"       , "/d/proj/bg/enkf/EnKF_OE/Run3/Observations/0061/RFT" , 1.0); */ /* E8 to ganger */
  /*ecl_rft_vector_fprintf_rft_obs(rft_vector , "E5A_RFT"  , "/d/proj/bg/enkf/EnKF_OE/Static/rft/E5A_rft"  , "/d/proj/bg/enkf/EnKF_OE/Run3/Observations/0063/RFT" , 1.0);*/
  /*ecl_rft_vector_fprintf_rft_obs(rft_vector , "E2" 	 , "/d/proj/bg/enkf/EnKF_OE/Static/rft/E2"       , "/d/proj/bg/enkf/EnKF_OE/Run3/Observations/0065/RFT" , 1.0);*/
  /*ecl_rft_vector_fprintf_rft_obs(rft_vector , "E7" 	 , "/d/proj/bg/enkf/EnKF_OE/Static/rft/E7"       , "/d/proj/bg/enkf/EnKF_OE/Run3/Observations/0070/RFT" , 1.0);*/
  /*ecl_rft_vector_fprintf_rft_obs(rft_vector , "E14W" 	 , "/d/proj/bg/enkf/EnKF_OE/Static/rft/E14W"     , "/d/proj/bg/enkf/EnKF_OE/Run3/Observations/0071/RFT" , 1.0);*/ /* E14W to ganger */
  /*ecl_rft_vector_fprintf_rft_obs(rft_vector , "E7A_RFT"  , "/d/proj/bg/enkf/EnKF_OE/Static/rft/E7A_rft"  , "/d/proj/bg/enkf/EnKF_OE/Run3/Observations/0074/RFT" , 1.0);*/
  /*ecl_rft_vector_fprintf_rft_obs(rft_vector , "E11A_RFT" , "/d/proj/bg/enkf/EnKF_OE/Static/rft/E11A_RFT" , "/d/proj/bg/enkf/EnKF_OE/Run3/Observations/0078/RFT" , 1.0);*/
  /*ecl_rft_vector_fprintf_rft_obs(rft_vector , "E12W" 	 , "/d/proj/bg/enkf/EnKF_OE/Static/rft/E12W"     , "/d/proj/bg/enkf/EnKF_OE/Run3/Observations/0082/RFT" , 1.0);*/
  /*ecl_rft_vector_fprintf_rft_obs(rft_vector , "E7A" 	 , "/d/proj/bg/enkf/EnKF_OE/Static/rft/E7A"      , "/d/proj/bg/enkf/EnKF_OE/Run3/Observations/0087/RFT" , 1.0);*/
  /*ecl_rft_vector_fprintf_rft_obs(rft_vector , "E15" 	 , "/d/proj/bg/enkf/EnKF_OE/Static/rft/E15"      , "/d/proj/bg/enkf/EnKF_OE/Run3/Observations/0088/RFT" , 1.0);*/
  /*ecl_rft_vector_fprintf_rft_obs(rft_vector , "E1A" 	 , "/d/proj/bg/enkf/EnKF_OE/Static/rft/E1A"      , "/d/proj/bg/enkf/EnKF_OE/Run3/Observations/0095/RFT" , 1.0);*/
  ecl_rft_vector_fprintf_rft_obs(rft_vector , "E6A" 	 , "/d/proj/bg/enkf/EnKF_OE/Static/rft/E6A"      , "/d/proj/bg/enkf/EnKF_OE/Run3/Observations/0103/RFT" , 1.0);
  /*ecl_rft_vector_fprintf_rft_obs(rft_vector , "E6B" 	 , "/d/proj/bg/enkf/EnKF_OE/Static/rft/E6B"      , "/d/proj/bg/enkf/EnKF_OE/Run3/Observations/0106/RFT" , 1.0);*/
  /*ecl_rft_vector_fprintf_rft_obs(rft_vector , "E3A_RFT"  , "/d/proj/bg/enkf/EnKF_OE/Static/rft/E3A_rft"  , "/d/proj/bg/enkf/EnKF_OE/Run3/Observations/0129/RFT" , 1.0);*/
  /*ecl_rft_vector_fprintf_rft_obs(rft_vector , "E14W" 	 , "/d/proj/bg/enkf/EnKF_OE/Static/rft/E14W"     , "/d/proj/bg/enkf/EnKF_OE/Run3/Observations/0245/RFT" , 1.0); */
  /*ecl_rft_vector_fprintf_rft_obs(rft_vector , "E8" 	 , "/d/proj/bg/enkf/EnKF_OE/Static/rft/E8"       , "/d/proj/bg/enkf/EnKF_OE/Run3/Observations/0246/RFT" , 1.0); */ /* E8 to ganger */
  return 0;
}
