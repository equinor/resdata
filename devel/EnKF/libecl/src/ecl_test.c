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
  /*  const double tvd[55] = {0.27258293E+04,   0.27365356E+04,   0.27514102E+04,   0.27539917E+04,
			  0.27565728E+04,   0.27591538E+04,   0.27617351E+04,   0.27643162E+04,
			  0.27668975E+04,   0.27694788E+04,   0.27720598E+04,   0.27746411E+04,
			  0.27765449E+04,   0.27777712E+04,   0.27789976E+04,   0.27823044E+04,
			  0.27827869E+04,   0.27832690E+04,   0.27837512E+04,   0.27842336E+04,
			  0.27847158E+04,   0.27851982E+04,   0.27863728E+04,   0.27872747E+04,
			  0.27881765E+04,   0.27890784E+04,   0.27899802E+04,   0.27908823E+04,
			  0.27917842E+04,   0.27926860E+04,   0.27935881E+04,   0.27944902E+04,
			  0.28107236E+04,   0.28117195E+04,   0.28127148E+04,   0.28143879E+04,
			  0.28167388E+04,   0.28190894E+04,   0.28214399E+04,   0.28237903E+04,
			  0.28261414E+04,   0.28284919E+04,   0.28308423E+04,   0.28331931E+04,
			  0.28355437E+04,   0.28377681E+04,   0.28398660E+04,   0.28419641E+04,
			  0.28440620E+04,   0.28461602E+04,   0.28482581E+04,   0.28503562E+04,
			  0.28524541E+04,   0.28545518E+04,   0.28566501E+04};
  int i[55];
  int j[55];
  int k[55];

  ecl_rft_vector_type * rft_vector = ecl_rft_vector_alloc(argv[1] , true);
  ecl_rft_vector_block(rft_vector , "B-43" , 55   , tvd , i , j  , k);
  ecl_rft_vector_free(rft_vector);
  */
  printf("Gjetter: %s \n",ecl_util_alloc_base_guess("/h/a152128/EnKF_ON/Run-FMT-Test/PriorEns2/tmpdir_0004"));
  return 0;
}
