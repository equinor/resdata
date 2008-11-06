#include <stdlib.h>
#include <ecl_rft_vector.h>
#include <util.h>





int main (int argc , char ** argv) {

  if (argc != 2)
    util_exit("Usage: rft.x BASENAME \n");
  {
    char * input_file = argv[1];
    char * rft_file   = NULL;
    ecl_file_type input_type;

    ecl_rft_vector_type * rft_vector;

    ecl_util_get_file_type( input_file , &input_type , NULL , NULL);
    if (input_type == ecl_rft_file)
      rft_file = util_alloc_string_copy(input_file);
    else {
      char  * base;
      char  * path;
      char  * rft_file_formatted   ;
      char  * rft_file_unformatted ;

      util_alloc_file_components( input_file , &path , &base , NULL);
      rft_file_formatted   = ecl_util_alloc_filename(path , base , ecl_rft_file , true  , -1);
      rft_file_unformatted = ecl_util_alloc_filename(path , base , ecl_rft_file , false , -1);
      
      if (util_file_exists( rft_file_formatted ) && util_file_exists( rft_file_unformatted )) {
	rft_file = util_alloc_string_copy( util_newest_file( rft_file_formatted , rft_file_unformatted));
	printf("Loading RFT vector from:%s \n",rft_file);
      } else if (util_file_exists( rft_file_formatted ))
	rft_file = util_alloc_string_copy( rft_file_formatted );
      else if (util_file_exists( rft_file_unformatted ))
	rft_file = util_alloc_string_copy( rft_file_unformatted );
      else 
	util_exit("Could not find RFT files: %s/%s \n",rft_file_formatted , rft_file_unformatted);
      
      free( rft_file_formatted );
      free( rft_file_unformatted );
      free( base );
      free( path );
    }
    
    rft_vector = ecl_rft_vector_alloc(rft_file);
    ecl_rft_vector_summarize( rft_vector , true);
    ecl_rft_vector_free( rft_vector );
    free( rft_file );
  }
}
  

