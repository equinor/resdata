#include <ecl_rft_file.h>
#include <ecl_rft_node.h>
#include <stringlist.h>
#include <util.h>


int main (int argc , char ** argv) {
  if (argc != 2)
    util_exit("I want one RFT file - try again \n");
  {
    const char * filename = argv[1];
    ecl_rft_file_type * rft_file = ecl_rft_file_alloc( filename );  
    stringlist_type   * wells    = ecl_rft_file_alloc_well_list( rft_file );
    
    printf("RFT file: %s has a total of %d entries and %d wells \n",filename , ecl_rft_file_get_size( rft_file), stringlist_get_size( wells ));
    {
      int iw;
      for (iw = 0; iw < stringlist_get_size( wells ); iw++) {
	const char * well = stringlist_iget(wells , iw);
	printf("  Well: %s has a total of %d RFTs in the file. \n",well , ecl_rft_file_get_well_occurences( rft_file , well ));
	{
	  int it;
	  for (it = 0; it < ecl_rft_file_get_well_occurences( rft_file , well ); it++) {
	    const ecl_rft_node_type * node = ecl_rft_file_iget_well_rft( rft_file , well , it);
	    time_t date  = ecl_rft_node_get_date( node );
	    {
	      int mday, year,month;
	      util_set_date_values( date , &mday , &month , &year);
	      printf("    RFT was recorded on %02d/%02d/%4d \n",mday,month,year);
	      {
		int num_cells = ecl_rft_node_get_size( node );
		int icell;
		printf("    A total of %d cells have been perforated \n",num_cells);
		for (icell = 0; icell < num_cells; icell++) {
		  int i,j,k;
		  ecl_rft_node_iget_ijk( node , icell , &i , &j , &k);
		  printf("      Cell: %d (%3d,%3d,%d) has pressure/depth:  %g/%g \n",icell , i,j,k,ecl_rft_node_iget_pressure( node , icell) , ecl_rft_node_iget_depth( node , icell));
		}
	      }
	    }
	  }
	}
      }
    }

    ecl_rft_file_free( rft_file );
  }
}
