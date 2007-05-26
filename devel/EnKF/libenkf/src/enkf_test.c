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
  
  config = enkf_config_alloc(4, 2);
  enkf_config_add_type(config , "MULTZ" , multz_config_alloc(100 , 100 , 100 , "MULTZ.INC" , "multz") , multz_config_free__ , multz_config_get_size__  );
  

  enkf_config_add_restart_type(config , "PRESSURE");
  enkf_config_add_restart_type(config , "SWAT");
  enkf_config_add_restart_type(config , "SGAS");
  enkf_config_add_restart_type(config , "RS");
  enkf_config_add_restart_type(config , "RV");


  state = enkf_state_alloc(config , "ECLIPSE");
  enkf_state_add_node(state , "MULTZ" , "MULTZ");

  
  enkf_state_iset_eclpath(state , 0 , "RunPATH");
  enkf_state_iset_eclpath(state , 1 , "tmpdir_0001");
  enkf_state_iset_eclpath(state , 1 , "tmpdir_0002");
  enkf_state_iset_eclpath(state , 1 , "tmpdir_0003");
  
  enkf_state_load_ecl_restart(state , "ECLIPSE.X0051" , true , false , 1);

  enkf_state_iset_enspath(state , 0 , "Ensemble");
  enkf_state_iset_enspath(state , 1 , "0001");
  enkf_state_iset_enspath(state , 2 , "1");
  enkf_state_iset_enspath(state , 3 , "Analyzed");
  enkf_state_iset_enspath(state , 3 , "Forecast");
  enkf_state_ens_write(state , all_types - ecl_static );
  
  enkf_state_iset_enspath(state , 3 , "Static");
  enkf_state_ens_write(state , ecl_static );
  enkf_state_ecl_write(state , "ECLIPSE.X0052" , all_types , false , true);
  
  enkf_state_free(state);
  enkf_config_free(config);
}
