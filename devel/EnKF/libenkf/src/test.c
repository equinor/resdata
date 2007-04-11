#include <stdlib.h>
#include <multz_config.h>
#include <ecl_config.h>
#include <ens_config.h>
#include <mem_config.h>
#include <multz_config.h>
#include <multz.h>
#include <multflt_config.h>
#include <multflt.h>
#include <enkf_node.h>
#include <enkf_state.h>
#include <work.h>
#include <timer.h>

/*

   gcc -m32 -I./ work.c timer.c test.c -L../../libutil/lib -L./ -lenkf -lutil

*/

int main(void) {
  enkf_node_type    * enkf_node;
  ens_config_type   * ens_config   = ens_config_alloc(100 , "Ensemble.1.0");
  ecl_config_type   * ecl_config   = ecl_config_alloc("RunECLIPSE");
  
  mem_config_type   * mem_config1  = mem_config_alloc(ens_config , ecl_config ,  "mem1" , "tmpdir1" );
  mem_config_type   * mem_config2  = mem_config_alloc(ens_config , ecl_config ,  "mem2" , "tmpdir2" );

  multflt_config_type * multflt_config = multflt_config_alloc(100 , "MULTFLT.INC" , "multflt");
  multz_config_type * multz_config     = multz_config_alloc(100 , 100 , 1000 , "MULTZ.INC" , "multz");
  multz_type      * multz1     = multz_alloc(mem_config1   , multz_config);
  multz_type      * multz2     = multz_alloc(mem_config2   , multz_config);
  multflt_type    * multflt    = multflt_alloc(mem_config1 , multflt_config);
  enkf_state_type * enkf_state = enkf_state_alloc(ens_config , ecl_config , "mem1" , "tmpdir1");


  ens_config_set_ext_path(ens_config , "Time0");
  {
    char *ens_file = multz_alloc_ensname(multz1);
    printf("Skal save til: %s \n",ens_file);
    free(ens_file);
  }

  {
    char *ens_file = multz_alloc_ensname(multz2);
    printf("Skal save til: %s \n",ens_file);
    free(ens_file);
  }
  
  mem_config_make_ecl_path(mem_config1);
  mem_config_make_ecl_path(mem_config2);

  ens_config_set_ext_path(ens_config , "Time2");
  {
    char *ens_file = multz_alloc_ensname(multz1);
    printf("Skal save til: %s \n",ens_file);
    free(ens_file);
  }

  {
    char *ecl_file = multz_alloc_eclname(multz1);
    printf("Eclipse fil saves til: %s \n",ecl_file);
    free(ecl_file);
  }

  {
    char *ecl_file = multz_alloc_eclname(multz2);
    printf("Eclipse fil saves til: %s \n",ecl_file);
    free(ecl_file);
  }

  multz_ecl_write(multz2);
  multz_ecl_write(multz2);

  /*
    Some leakage here ...
  */
  enkf_node = enkf_node_alloc(multz2 , NULL , multz_ecl_write__  ,  multz_ens_read__ , multz_ens_write__ , multz_sample__ , multflt_free__);
  enkf_state_add_node(enkf_state , enkf_node);
  enkf_node = enkf_node_alloc(multflt , NULL , multflt_ecl_write__  ,  multflt_ens_read__ , multflt_ens_write__ , multflt_sample__ , multflt_free__);
  enkf_state_add_node(enkf_state , enkf_node);
  


  enkf_node_ecl_write(enkf_node);
  enkf_node_sample(enkf_node);
  enkf_node_ens_write(enkf_node);
  enkf_node_sample(enkf_node);
  enkf_node_ens_read(enkf_node);
  enkf_node_sample(enkf_node);
  enkf_node_ens_read(enkf_node);
  
  {
    int i;
    char ens_path[32];
    char ecl_path[32];

    work_type        * work;
    timer_type       **timer_list;
    multz_type       ** multz_list;
    mem_config_type  ** mem_list;
    enkf_node_type   ** ens;

    timer_list = malloc(  3 * sizeof *timer_list);
    multz_list = malloc(100 * sizeof *multz_list);
    mem_list   = malloc(100 * sizeof *mem_list);
    ens        = malloc(100 * sizeof *ens);
    
    timer_list[0] = timer_alloc("Sampling" , true);
    timer_list[1] = timer_alloc("eclipse"  , true);
    timer_list[2] = timer_alloc("ensemble" , true);

    for (i=0; i < 100; i++) {
      sprintf(ens_path , "mem%d" , i+1);
      sprintf(ecl_path , "tmpdir%d" , i+1);
      mem_list[i]   = mem_config_alloc(ens_config , ecl_config , ens_path , ecl_path);
      multz_list[i] = multz_alloc(mem_list[i] , multz_config);
      ens[i]        = enkf_node_alloc(multz_list[i] , NULL , multz_ecl_write__  ,  multz_ens_read__ , multz_ens_write__ , multz_sample__ , multz_free__ );
    }
    enkf_state_add_node(enkf_state , ens[0]);
    enkf_state_add_node(enkf_state , ens[2]);

    for (i=0; i < 100; i++) {
      timer_start(timer_list[0]);
      enkf_node_sample(ens[i]);
      timer_stop(timer_list[0]);
2
      mem_config_make_ecl_path(mem_list[i]);

      timer_start(timer_list[1]);
      enkf_node_ecl_write(ens[i]);
      timer_stop(timer_list[1]);


      timer_start(timer_list[2]);
      enkf_node_ens_write(ens[i]);
      timer_stop(timer_list[2]);
      
      printf("saver: %d \n",i);
    }
    work = work_alloc_double(10000 , __func__);
    timer_list_report((const timer_type **) timer_list , 3 , stdout , work);
    enkf_state_ecl_write(enkf_state);
  }
}



