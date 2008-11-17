#include <ecl_fstate.h>
#include <ecl_kw.h>
#include <ecl_block.h>
#include <ecl_grid.h>
#include <util.h>



int main(int argc , char ** argv) {
  ecl_grid_type * ecl_grid = NULL;
  int num_restart_files;
  char ** restart_files;
  {
    const char * first_arg = argv[1];
    char * base;
    char * path;
    ecl_file_type file_type;

    ecl_util_get_file_type(first_arg , &file_type , NULL , NULL);
    if (file_type == ecl_grid_file || file_type == ecl_egrid_file)
      ecl_grid = ecl_grid_alloc( first_arg , true );

    util_alloc_file_components(first_arg , &path , &base , NULL);
    
    ecl_util_alloc_restart_files(path , base , &restart_files , &num_restart_files , NULL , NULL);
    ecl_util_get_file_type(restart_files[0] , &file_type , NULL , NULL);
    {
      const char * kw = "SGAS";
      ecl_fstate_type * fstate = ecl_fstate_fread_alloc(num_restart_files , (const char **) restart_files , file_type , true , true);
      int num_blocks = ecl_fstate_get_size(fstate);
      int iblock;
      for (iblock = 0; iblock  < num_blocks; iblock++) {
	ecl_block_type * ecl_block = ecl_fstate_iget_block( fstate , iblock);
	time_t   sim_time          = ecl_block_get_sim_time(ecl_block);    
	ecl_kw_type * ecl_kw = ecl_block_iget_kw(ecl_block , kw , 0);
	int  day,month,year;

	util_set_date_values( sim_time , &day , &month , &year);
	printf("%02d/%02d/%04d   %03d  %g \n",day , month , year , iblock , ecl_kw_iget_float(ecl_kw , 6754));
      }
      
      ecl_fstate_free(fstate);
    }
  }
}

