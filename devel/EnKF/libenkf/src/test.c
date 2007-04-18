#include <stdlib.h>
#include <multz_config.h>
#include <ecl_config.h>
#include <ens_config.h>
#include <multz_config.h>
#include <multz.h>
#include <multflt_config.h>
#include <multflt.h>
#include <enkf_node.h>
#include <enkf_state.h>
#include <work.h>
#include <timer.h>

/*
   gcc -m32 -I./ -I../../libhash/include -I../../libutil/include test.c -L ../../libutil/lib -L./ -L../../libhash/lib -lenkf -lutil -lhash -lm
*/

int main(void) {
  const int ens_size = 100;
  enkf_node_type    * enkf_node;
  ens_config_type   * ens_config   = ens_config_alloc(ens_size , "Ensemble.1.0");
  ecl_config_type   * ecl_config   = ecl_config_alloc("RunECLIPSE");
  enkf_state_type  ** ensemble;
  multz_type       ** multz_list;
  multflt_type     ** multflt_list;
  int                 i;
  char                ens_path[128];
  char                ecl_path[128];

  multflt_config_type * multflt_config = multflt_config_alloc(100             , "MULTFLT.INC" , "multflt");
  multz_config_type   * multz_config   = multz_config_alloc  (100 , 100 , 100 , "MULTZ.INC" , "multz");
  ens_config_set_ext_path(ens_config , "Time0");
  ensemble     = malloc(ens_size   * sizeof *ensemble);
  multz_list   = malloc(ens_size   * sizeof *multz_list);
  multflt_list = malloc(ens_size   * sizeof *multflt_list);
  
  
  for (i=0; i < ens_size; i++) {
    sprintf(ens_path , "mem%d" , i+1);
    sprintf(ecl_path , "tmpdir%d" , i+1);
    ensemble[i]     = enkf_state_alloc(ens_config , ecl_config , ens_path , ecl_path);
    multz_list[i]   = multz_alloc(ensemble[i] , multz_config);
    multflt_list[i] = multflt_alloc(ensemble[i] , multflt_config);
    
    enkf_state_add_node(ensemble[i] , "MULTZ"   , multz_list[i]   , NULL , multz_ecl_write__    ,  multz_ens_read__   , multz_ens_write__   , multz_sample__   , multz_free__   );
    enkf_state_add_node(ensemble[i] , "MULTFLT" , multflt_list[i] , NULL , multflt_ecl_write__  ,  multflt_ens_read__ , multflt_ens_write__ , multflt_sample__ , multflt_free__ );
  }
  printf("Ferdig .... \n");

  
  for (i=0; i < ens_size; i++) {
    enkf_state_make_ecl_path(ensemble[i]);
    enkf_state_sample(ensemble[i]);
    printf("i: %d \n",i);
    enkf_state_get_node(ensemble[i] , "MULTZ");
    enkf_state_get_node(ensemble[i] , "MULTFLT");
    enkf_state_ecl_write(ensemble[i]);
    enkf_state_del_node(ensemble[i] , "MULTZ");
    enkf_state_ens_write(ensemble[i]);
  }

  
  return 0;
}



