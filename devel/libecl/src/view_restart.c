#include <ecl_fstate.h>
#include <ecl_kw.h>
#include <ecl_block.h>
#include <ecl_grid.h>
#include <util.h>



int main(int argc , char ** argv) {
  if (argc < 3)
    util_exit("Usage: <BASE|GRID>  PRESSURE:10,5,17  SWAT:10,5,7   ...\n");
  {
    ecl_grid_type * ecl_grid = NULL;
    int num_restart_files;
    char ** restart_files;
    {
      const char * first_arg = argv[1];
      char * base;
      char * path;
      ecl_file_enum file_type;
      
      ecl_util_get_file_type(first_arg , &file_type , NULL , NULL);
      if (file_type == ecl_grid_file || file_type == ecl_egrid_file)
	ecl_grid = ecl_grid_alloc( first_arg , true );
      
      util_alloc_file_components(first_arg , &path , &base , NULL);
      
      ecl_util_alloc_restart_files(path , base , &restart_files , &num_restart_files , NULL , NULL);
      ecl_util_get_file_type(restart_files[0] , &file_type , NULL , NULL);
      {
	const char ** arg_list = (const char **) &argv[2];
	char       ** kw_list;
	int         * index_list;
	int num_kw = argc - 2;
	ecl_fstate_type * fstate = ecl_fstate_fread_alloc(num_restart_files , (const char **) restart_files , file_type , true , true);
	int num_blocks = ecl_fstate_get_size(fstate);
	int iblock;

	/* 
	   Finding the indices ... 
	*/
	kw_list    = util_malloc(num_kw * sizeof * kw_list    , __func__);
	index_list = util_malloc(num_kw * sizeof * index_list , __func__); 
	{
	  char ** tokens;
	  int     num_tokens;
	  for (int ikw = 0; ikw < num_kw; ikw++) {
	    util_split_string(arg_list[ikw] , ":" , &num_tokens , &tokens);
	    if (num_tokens != 2)
	      util_exit("Failed to parse \"%s\" as KEWYORD:INDEX \n", arg_list[ikw]);
	    kw_list[ikw] = util_alloc_string_copy( tokens[0] );
	    {
	      int   *ijk , num_coord;
	      ijk = util_sscanf_alloc_active_list( tokens[1] , &num_coord );
	      if (ecl_grid == NULL) {
		if (num_coord != 1)
		  util_exit("Failed to extract one integer from: %s \n",tokens[1]);
		else
		  index_list[ikw] = ijk[0];
	      } else {
		if (num_coord == 1) 
		  index_list[ikw] = ecl_grid_get_active_index_from_global(ecl_grid , ijk[0]);
		else if (num_coord == 3) {
		  int i = ijk[0] - 1;
		  int j = ijk[1] - 1;
		  int k = ijk[2] - 1;
		  
		  index_list[ikw] = ecl_grid_get_active_index(ecl_grid , i,j,k);
		} else 
		  util_exit("Failed to parse \"%s\" as one or three integers.\n",tokens[1]);
	      }
	      free(ijk);
	    }
	      

	    index_list[ikw] = 100; /* ... */
	    util_free_stringlist(tokens , num_tokens);
	  }
	}

	for (iblock = 0; iblock  < num_blocks; iblock++) {
	  ecl_block_type * ecl_block = ecl_fstate_iget_block( fstate , iblock);
	  time_t   sim_time          = ecl_block_get_sim_time(ecl_block);    
	  int      day,month,year , report_step;
	  double   sim_days;
	  
	  sim_days = ecl_block_get_sim_days(ecl_block);
	  report_step = ecl_block_get_report_nr( ecl_block );
	  util_set_date_values( sim_time , &day , &month , &year);
	  printf("%04d   %02d/%02d/%04d   %9.3f  ",report_step , day , month , year , sim_days );	  
	  
	  for (int ikw = 0; ikw < num_kw; ikw++) {
	    ecl_kw_type * ecl_kw = ecl_block_iget_kw(ecl_block , kw_list[ikw] , 0);
	    printf(" %14.6f ",   ecl_kw_iget_float(ecl_kw , index_list[ikw]));
	  }
	  printf("\n");
	}
	ecl_fstate_free(fstate);
      }
    }
  }
}
