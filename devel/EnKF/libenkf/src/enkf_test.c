#include <stdlib.h>
#include <enkf_util.h>
#include <enkf_state.h>
#include <enkf_node.h>
#include <enkf_config.h>
#include <multz_config.h>
#include <multz.h>
#include <equil_config.h>
#include <equil.h>
#include <ecl_kw.h>
#include <ecl_block.h>
#include <enkf_path.h>
#include <enkf_types.h>
#include <field_config.h>
#include <well_config.h>


int main(void) {
  enkf_config_type  * config;
  enkf_state_type   * state;
  const int *index_map;
  int nx,ny,nz,active_size;

  index_map = field_config_alloc_index_map1("ECLIPSE.EGRID" , true , &nx , &ny , &nz , &active_size);

  config = enkf_config_alloc(4, 2 , true);
  enkf_config_add_type(config , "MULTZ" , 
		       parameter , MULTZ, 
		       multz_config_alloc(100 , 100 , 100 , "MULTZ.INC" , "multz") , multz_config_free__ , multz_config_get_serial_size__  );

  enkf_config_add_type(config , "EQUIL" , 
		       parameter , EQUIL, 
		       equil_config_alloc(10 , true , true , "EQUIL.INC" , "equil") , equil_config_free__ , equil_config_get_serial_size__  );
  
  
  enkf_config_add_type(config , "SWAT"  , ecl_restart , FIELD , 
		       field_config_alloc("SWAT" , ecl_float_type   , nx , ny , nz , active_size , index_map , 1 , NULL , "SWAT")     , 
		       field_config_free__ , field_config_get_serial_size__);

  enkf_config_add_type(config , "PRESSURE" , ecl_restart , FIELD , 
		       field_config_alloc("PRESSURE"  , ecl_float_type , nx , ny , nz , active_size , index_map , 1 , NULL , "Pressure") , 
		       field_config_free__ , field_config_get_serial_size__);

  enkf_config_add_type(config , "SGAS"  , ecl_restart , FIELD , 
		       field_config_alloc("SGAS" , ecl_float_type    , nx , ny , nz , active_size , index_map , 1 , NULL , "SGAS")     , 
		       field_config_free__ , field_config_get_serial_size__);

  enkf_config_add_type(config , "RS"     , ecl_restart , FIELD , 
		       field_config_alloc("RS"  , ecl_float_type        , nx , ny , nz , active_size , index_map , 1 , NULL , "RS")       , 
		       field_config_free__ , field_config_get_serial_size__);
  
  enkf_config_add_type(config , "RV"    , ecl_restart , FIELD , 
		       field_config_alloc("RV"    , ecl_float_type       , nx , ny , nz , active_size , index_map , 1 , NULL , "RV")       , 
		       field_config_free__ , field_config_get_serial_size__);
  
  enkf_config_add_well(config , "B-33A" , "well.ens" , 3 , (const char *[3]) {"WGPR" , "WWPR" , "WOPR"});
  
  
  state = enkf_state_alloc(config , "ECLIPSE" , false);
  enkf_state_add_node(state , "MULTZ"); 
  enkf_state_add_node(state , "EQUIL");

  enkf_state_iset_eclpath(state , 0 , "RunPATH");
  enkf_state_iset_eclpath(state , 1 , "tmpdir_0001");
  
  enkf_state_load_ecl_summary(state , false , 51);
  enkf_state_load_ecl_restart(state , false , 51);

  enkf_state_iset_enspath(state , 0 , "Ensemble");
  enkf_state_iset_enspath(state , 1 , "0001");
  enkf_state_iset_enspath(state , 2 , "1");
  enkf_state_iset_enspath(state , 3 , "Forecast");
  enkf_state_ens_write(state , all_types - ecl_static );
  
  enkf_state_iset_enspath(state , 3 , "Static");
  enkf_state_swapout(state , ecl_static);
  
  enkf_state_iset_eclpath(state , 1 , "tmpdir_0002");
  enkf_state_ecl_write(state , all_types , 51);
  
  enkf_state_swapin(state , ecl_static);

  {
    const int serial_size = enkf_config_get_serial_size(config);
    double * serial_data = calloc(serial_size , sizeof *serial_data);
    enkf_state_serialize(state , serial_data);
    free(serial_data);
    printf("serial size: %d \n",serial_size);
  }
  enkf_state_free(state);
  enkf_config_free(config);
}
