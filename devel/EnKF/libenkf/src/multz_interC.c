#include <multz.h>
#include <multz_config.h>
#include <util.h>

static multz_config_type  * MULTZ_CONFIG = NULL;
static multz_type        ** MULTZ_LIST        = NULL;

/*****************************************************************/

void multz_inter_init_(const char * _config_file, const int * config_file_len , const int * ens_size, const int * n_multz, const int *nx , const int * ny , const int * nz) {
  int iens;
  char * config_file = util_alloc_cstring(_config_file , config_file_len);
  MULTZ_CONFIG = multz_config_fscanf_alloc(config_file , *nx , *ny , *nz , "MULTZ.INC" , NULL);
  if (*n_multz != multz_config_get_serial_size(MULTZ_CONFIG)) {
    fprintf(stderr,"%s: size mismatch - aborting \n",__func__);
    abort();
  }


  MULTZ_LIST = malloc(*ens_size * sizeof * MULTZ_LIST);
  
  for (iens = 0; iens < *ens_size; iens++) 
    MULTZ_LIST[iens] = multz_alloc(MULTZ_CONFIG);

  free(config_file);
}



void multz_inter_ecl_write_(const char *_path , const int * path_len , const double *data , const int *iens) {
  char *path = util_alloc_cstring(_path , path_len);

  multz_set_data(MULTZ_LIST[(*iens)] , data);
  multz_ecl_write(MULTZ_LIST[(*iens)] , path);
  
  free(path);
}



void multz_inter_sample(const int * iens) {
  multz_sample(MULTZ_LIST[*iens]);
}
