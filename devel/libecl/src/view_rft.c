#include <stdlib.h>
#include <ecl_rft_file.h>
#include <ecl_util.h>
#include <util.h>





int main (int argc , char ** argv) {

  if (argc != 2)
    util_exit("Usage: rft.x BASENAME \n");
  {
    char * input_file = argv[1];
    char * file_name   = NULL;
    ecl_file_enum input_type;

    ecl_rft_file_type * rft_file;

    ecl_util_get_file_type( input_file , &input_type , NULL , NULL);
    if (input_type == ECL_RFT_FILE)
      file_name = util_alloc_string_copy(input_file);
    else {
      char  * base;
      char  * path;
      char  * rft_file_formatted   ;
      char  * rft_file_unformatted ;

      util_alloc_file_components( input_file , &path , &base , NULL);
      rft_file_formatted   = ecl_util_alloc_filename(path , base , ECL_RFT_FILE , true  , -1);
      rft_file_unformatted = ecl_util_alloc_filename(path , base , ECL_RFT_FILE , false , -1);
      
      if (util_file_exists( rft_file_formatted ) && util_file_exists( rft_file_unformatted )) 
	file_name = util_alloc_string_copy( util_newest_file( rft_file_formatted , rft_file_unformatted));
      else if (util_file_exists( rft_file_formatted ))
	file_name = util_alloc_string_copy( rft_file_formatted );
      else if (util_file_exists( rft_file_unformatted ))
	file_name = util_alloc_string_copy( rft_file_unformatted );
      else 
	util_exit("Could not find RFT files: %s/%s \n",rft_file_formatted , rft_file_unformatted);
      
      free( rft_file_formatted );
      free( rft_file_unformatted );
      free( base );
      free( path );
    }
    
    rft_file = ecl_rft_file_alloc(file_name);
    ecl_rft_file_summarize( rft_file , true);
    ecl_rft_file_free( rft_file );
  }
}
  

