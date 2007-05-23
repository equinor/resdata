#include <stdlib.h>
#include <enkf_util.h>
#include <enkf_state.h>
#include <enkf_config.h>
#include <multz_config.h>
#include <multz.h>
#include <equil_config.h>
#include <equil.h>
#include <ecl_kw.h>
#include <ecl_block.h>
#include <enkf_path.h>


int main(void) {
  enkf_config_type  * config;
  enkf_state_type   * state;
  
  config = enkf_config_alloc(3 , 2);
  enkf_config_add_type(config , "MULTZ" , multz_config_alloc(100 , 100 , 100 , "MULTZ.INC" , "multz") , multz_config_free__);

  enkf_config_iset_enspath(config , 0 , "Ensemble");
  enkf_config_iset_enspath(config , 1 , "0001");
  enkf_config_iset_enspath(config , 2 , "Analyzed");
  enkf_config_iset_enspath(config , 2 , "Forecast");
  enkf_config_iset_enspath(config , 2 , "Static");
  
  state = enkf_state_alloc(config);
  enkf_state_add_node(state , "MULTZ" , "MULTZ");
  
  enkf_state_iset_eclpath(state , 0 , "RunPATH");
  enkf_state_iset_eclpath(state , 1 , "tmpdir_0001");
  enkf_state_iset_eclpath(state , 1 , "tmpdir_0002");
  enkf_state_iset_eclpath(state , 1 , "tmpdir_0003");

  printf("Skal ta ecl_restart load \n");
  enkf_state_load_ecl_restart(state , "ECLIPSE.X0051" , true , false , 1);
  enkf_state_ens_write(state , "Ens/Test" , all_types);
  
  enkf_state_free(state);
  enkf_config_free(config);

  
}
