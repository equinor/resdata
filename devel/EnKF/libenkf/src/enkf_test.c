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


int main(void) {
  enkf_config_type * enkf_config;
  enkf_state_type  * enkf_state;
  multz_type       * multz;
  
  multz_config_type * multz_config = multz_config_alloc(100 , 100 , 10 , "MULTZ.INC" , "multz");

  enkf_config = enkf_config_alloc("Ensemble" , "0000/Forecast" , "RunECLIPSE");
  enkf_state = enkf_state_alloc(enkf_config);


  enkf_config_add_type( enkf_config , "EQUIL" , equil_config_alloc(10 , "EQUIL.INC" , "equil") , equil_config_set_ensfile__ , equil_config_set_eclfile__ ,  equil_config_free__  );
  enkf_config_add_type( enkf_config , "MULTZ" , multz_config                                   , multz_config_set_ensfile__ , multz_config_set_eclfile__ ,  multz_config_free__);
  

  enkf_state_add_node(enkf_state , parameter , "MULTZ" , multz_alloc(enkf_state , (const multz_config_type *) enkf_config_get_node_value(enkf_config , "MULTZ")) , 
		      multz_ecl_write__ ,
		      multz_ens_read__  ,
		      multz_ens_write__ ,
		      multz_sample__    , 
		      multz_free__);
  

  enkf_state_add_node(enkf_state , parameter , "EQUIL" , equil_alloc(enkf_state , (const equil_config_type *) enkf_config_get_node_value(enkf_config , "EQUIL")) , 
		      equil_ecl_write__ ,
		      equil_ens_read__  ,
		      equil_ens_write__ ,
		      equil_sample__    , 
		      equil_free__);
  

  enkf_config_add_enkf_kw(enkf_config , "SWAT    ");
  enkf_config_add_enkf_kw(enkf_config , "SGAS    ");
  enkf_config_add_enkf_kw(enkf_config , "RS      ");
  

  enkf_state_sample(enkf_state , parameter);
  enkf_state_ens_write(enkf_state , all_types);
  enkf_config_set_ens_path(enkf_config , "0000/Analysed");
  enkf_state_ens_write(enkf_state , all_types);
  
  enkf_config_set_ecl_path(enkf_config , "tmpdir_0001");
  enkf_state_ecl_write(enkf_state , all_types , false , false , NULL);
  
  enkf_config_set_ecl_path(enkf_config , "SeismicMonitor/tmpdir_0002");
  enkf_state_ecl_write(enkf_state , all_types , false , false , NULL);
  
  enkf_config_set_ens_root_path(enkf_config , "Ens2");
  enkf_state_ens_write(enkf_state , all_types);

  {
    bool at_eof;
    fortio_type * fortio = fortio_open("ECLIPSE.X0051" , "r" , true);
    ecl_block_type *ecl_block = ecl_block_alloc(0 , 10 , false , true , NULL);
    ecl_block_fread(ecl_block , fortio , &at_eof , false);
    fortio_close(fortio);

    enkf_state_ecl_read(enkf_state , ecl_block);
    printf("Har loadet en block .... \n");
    
    ecl_block_free(ecl_block);
  }
    
  enkf_config_free(enkf_config);
  enkf_state_free(enkf_state);

  return 1;
}
