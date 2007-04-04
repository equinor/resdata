#include <stdlib.h>
#include <multz_config.h>
#include <ecl_config.h>
#include <ens_config.h>
#include <mem_config.h>
#include <multz_config.h>
#include <multz_mem.h>

/*

   gcc -m32 -I./ test.c -L../../libutil/lib -L./ -lenkf -lutil

*/

int main(void) {
  ens_config_type   * ens_config   = ens_config_alloc(100 , "Ensemble.1.0");
  ecl_config_type   * ecl_config   = ecl_config_alloc("RunECLIPSE");
  
  mem_config_type   * mem_config1  = mem_config_alloc(ens_config , ecl_config ,  "mem1" , "tmpdir1" );
  mem_config_type   * mem_config2  = mem_config_alloc(ens_config , ecl_config ,  "mem2" , "tmpdir2" );

  multz_config_type * multz_config = multz_config_alloc(100 , 100 , 10 , "MULTZ.INC" , "multz");
  multz_mem_type    * multz_mem1   = multz_mem_alloc(mem_config1 , multz_config);
  multz_mem_type    * multz_mem2   = multz_mem_alloc(mem_config2 , multz_config);
  
  ens_config_set_ext_path(ens_config , "Time0");
  {
    char *ens_file = multz_mem_alloc_ensname(multz_mem1);
    printf("Skal save til: %s \n",ens_file);
    free(ens_file);
  }

  {
    char *ens_file = multz_mem_alloc_ensname(multz_mem2);
    printf("Skal save til: %s \n",ens_file);
    free(ens_file);
  }
  
  mem_config_make_ecl_path(mem_config1);
  mem_config_make_ecl_path(mem_config2);

  ens_config_set_ext_path(ens_config , "Time2");
  {
    char *ens_file = multz_mem_alloc_ensname(multz_mem1);
    printf("Skal save til: %s \n",ens_file);
    free(ens_file);
  }

  {
    char *ecl_file = multz_mem_alloc_eclname(multz_mem1);
    printf("Eclipse fil saves til: %s \n",ecl_file);
    free(ecl_file);
  }

  {
    char *ecl_file = multz_mem_alloc_eclname(multz_mem2);
    printf("Eclipse fil saves til: %s \n",ecl_file);
    free(ecl_file);
  }
  
}



