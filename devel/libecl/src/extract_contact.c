#include <ecl_grid.h>
#include <ecl_util.h>
#include <fortio.h>
#include <stdbool.h>
#include <ecl_kw.h>
#include <util.h>
#include <string.h>
#include <int_vector.h>
#include <double_vector.h>
#include <ecl_file.h>
#include <math.h>


#define SDP_url          "http://sdp.statoil.no/wiki/index.php/Res:Available_forward_models#EXTRACT_OWC"
#define DETECTION_LIMIT  0.20



void extract_contact_peak(const ecl_kw_type   * swat1    , 
			  const ecl_kw_type   * swat2    , 
			  const ecl_grid_type * ecl_grid , 
			  const int_vector_type * ilist  , 
			  const int_vector_type * jlist  , 
			  const char * target_file,
			  const char * surf_file) {

  const char * profile_path = "Profiles";
  int index,k,nz,active_size,nx,ny;
  double * diff;
  double * z , * sw1 , * sw2;
  int k_diffmax = -100;
  double   diffmax;
  FILE * target_stream = util_fopen( target_file , "w"); 
  FILE * surf_stream   = NULL;

  util_make_path(profile_path);
  if (surf_file != NULL)
    surf_stream = util_fopen( surf_file , "w");

  ecl_grid_get_dims( ecl_grid , &nx, &ny , &nz , &active_size);
  diff = util_malloc( nz * sizeof * diff , __func__);
  z    = util_malloc( nz * sizeof * z    , __func__);
  sw1  = util_malloc( nz * sizeof * sw1  , __func__);
  sw2  = util_malloc( nz * sizeof * sw2  , __func__);
  
  for (index = 0; index < int_vector_size( ilist ); index++) {
    double z_sum = 0;
    double w_sum = 0;
    int i 	 = int_vector_iget( ilist , index);
    int j 	 = int_vector_iget( jlist , index);
  
    diffmax = -1000;
    for (k=0; k < nz; k++) {
      int active_index = ecl_grid_get_active_index3( ecl_grid , i , j , k);
      if (active_index >= 0) {
	sw1[k] = ecl_kw_iget_as_double( swat1 , active_index );
	sw2[k] = ecl_kw_iget_as_double( swat2 , active_index );

	diff[k] = (sw2[k] - sw1[k]);  /* Absolute difference in saturation */
      } else
	diff[k] = -1;           /* Not active */
      
      {
	double xpos,ypos;
	ecl_grid_get_pos3(ecl_grid , i , j , k , &xpos , &ypos , &z[k]);
	if (diff[k] > 0) {
	  z_sum += z[k] * diff[k];
	  w_sum += diff[k];
	  if (diff[k] > diffmax) {
	    diffmax = diff[k];
	    k_diffmax = k;
	  }
	}
      }
    }
    
    
    /* Writing the profile - just for debugging/inspection */
    {
      char * filename = util_alloc_sprintf("%s/%s_profile_%d.%d" , profile_path , target_file , i ,j);
      FILE * stream   = util_fopen(filename , "w");
      
      for (k=0; k < nz; k++) 
	if (fabs(diff[k]) < 1)
	  fprintf(stream , "%4d   %6.4f  %6.4f   %7.4f   %12.6f   \n",k+1 , sw1[k] , sw2[k] , diff[k] , z[k]);
      
      fclose(stream);
      free(filename);
    }
    
    if (w_sum > 0) {
      double z_owc = z_sum / w_sum;
      if (surf_stream != NULL) {
	double xpos,ypos,zpos;
	ecl_grid_get_pos3(ecl_grid , i , j , k_diffmax , &xpos , &ypos , &zpos);
	fprintf(surf_stream   , "%18.6f  %18.6f  %12.6f  \n",xpos,ypos,z_owc);
      }
      fprintf(target_stream , "%g\n", z_owc);
    } else
      fprintf(target_stream , "0.0\n");
    
  }
  if (surf_stream != NULL)
    fclose(surf_stream);
  fclose( target_stream );
  free( diff );
  free( sw1 );
  free( sw2 );
  free( z );
}



void extract_contact(const ecl_kw_type   * swat1    , 
		     const ecl_kw_type   * swat2    , 
		     const ecl_grid_type * ecl_grid , 
		     const int_vector_type * ilist  , 
		     const int_vector_type * jlist  , 
		     const char * target_file,
		     const char * surf_file) {

  int index,nz,active_size,nx,ny;
  double * sw1 , * sw2;
  FILE * target_stream = util_fopen( target_file , "w"); 
  FILE * surf_stream   = NULL;

  if (surf_file != NULL)
    surf_stream = util_fopen( surf_file , "w");

  ecl_grid_get_dims( ecl_grid , &nx, &ny , &nz , &active_size);
  sw1  = util_malloc( nz * sizeof * sw1  , __func__);
  sw2  = util_malloc( nz * sizeof * sw2  , __func__);
  
  for (index = 0; index < int_vector_size( ilist ); index++) {
    int i 	 = int_vector_iget( ilist , index);
    int j 	 = int_vector_iget( jlist , index);

    {
      int k = 0;
      double owc = -1;
      double xpos,ypos;

      while (1) {
	int active_index = ecl_grid_get_active_index3( ecl_grid , i , j , k);
	if (active_index >= 0) {
	  double dk;
	  sw1[k] = ecl_kw_iget_as_double( swat1 , active_index );
	  sw2[k] = ecl_kw_iget_as_double( swat2 , active_index );
	  
	  dk = sw2[k] - sw1[k];
	  if (dk > DETECTION_LIMIT) {
	    double zk,zk1,dk1;
	    ecl_grid_get_pos3(ecl_grid , i , j , k , &xpos , &ypos , &zk);
	    owc = zk;

	    if (k > 0) {
	      /* Try a basic linear interpolation. */
	      int prev_active_index = ecl_grid_get_active_index3( ecl_grid , i , j , k - 1);
	      if (prev_active_index >= 0) {
		ecl_grid_get_pos3(ecl_grid , i , j , k - 1 , &xpos , &ypos , &zk1);
		dk1 = sw2[k - 1] - sw1[k -1];
		
		owc = (0.20 - dk) * (zk - zk1)/(dk - dk1) + zk;
	      }
	    }
	    break;
	  }
	}
	k++;
	if (k == nz) 
	  break;
      }
  
      if (surf_stream != NULL) 
	if (owc > 0)
	  fprintf(surf_stream   , "%18.6f  %18.6f  %12.6f  \n",xpos,ypos,owc);

      fprintf(target_stream , "%g\n", owc);
    }
  }
  fclose( target_stream );
  if (surf_stream != NULL)
    fclose(surf_stream);
  free( sw1 );
  free( sw2 );
}




static ecl_file_type * load_restart_section( const char * unif_restart_file , int report_nr) {

  ecl_file_type * section;
  printf("Loading report step %04d from: %s .....", report_nr , unif_restart_file); fflush(stdout);
  section = ecl_file_fread_alloc_unrst_section(  unif_restart_file , report_nr , true );
  if (section == NULL)
    util_exit("exit: failed to load report_step:%d from file:%s.\n", report_nr , unif_restart_file);
  printf("\n");
  return section;

}


static void usage() {
  printf("*****************************************************************\n");
  printf("The extract_owc.x program needs the following input on the command-line\n");
  printf("\n");
  printf("   1. The name of an ECPLISE grid file.\n");
  printf("\n");
  printf("   2. The name of ECLIPSE unified restart file and the report steps for\n");
  printf("      base and monitor surveys respectively.\n");
  printf("\n");
  printf("         or\n");
  printf("\n");
  printf("   2. The name of two non-unified ECLIPSE restart files from the base\n");
  printf("      and monitor times respectively.\n");
  printf("\n");
  printf("   3. A file with (i,j) index pairs for the region you are interested\n");
  printf("      in.\n");
  printf("\n");
  printf("   4. [Output:] The name of a file where OWC results are stored, this is just for enkf.\n");
  printf("\n");
  printf("   5. [Output:] The name of a file where columns \"x y owc\" are stored, this can be\n");
  printf("      loaded into e.g. RMS to see the new estimated contact.\n");
  printf("\n");
  printf("Example1:\n");
  printf("---------\n");
  printf("\n");
  printf("  bash%% extract_contact.x  ECLIPSE.EGRID   ECLIPSE.UNRST   0  100   IJ_file enkf_results   results.xyz\n");
  printf("\n");
  printf(" \n");
  printf("\n");
  printf("Example2:\n");
  printf("---------\n");
  printf("\n");
  printf("  bash%% extract_contact.x  ECLIPSE.GRID   ECLIPSE.X0000   ECLIPSE.X0075   IJ_file enkf_results   results.xyz\n");
  printf("\n");
  printf("\n");
  printf("For more documentation look at the web-page: %s\n" , SDP_url);
  printf("\n");
  printf("*****************************************************************\n");
  exit(1);
}



int main (int argc , char ** argv) {
  char * result_file , * surf_file;
  int_vector_type * ilist    = int_vector_alloc(100 , -1);
  int_vector_type * jlist    = int_vector_alloc(100 , -1);
  ecl_grid_type * grid         = NULL;

  int arg_offset,nx,ny,nz,active_size;
  
  ecl_kw_type 	* swat1 = NULL;
  ecl_kw_type 	* swat2 = NULL;
  
  
  /* Loading the grid */
  if (argc > 1) {
    printf("Loading grid file:%s ......" , argv[1]); fflush(stdout);
    grid = ecl_grid_alloc( argv[1] , true );    
    printf("\n");
  } else
    usage();

  
  /* Loading the swat keywords. */
  {
    ecl_file_type * section1 = NULL;
    ecl_file_type * section2 = NULL;
    
    if (argc > 2) {
      ecl_file_enum file_type;
      ecl_util_get_file_type( argv[2] , &file_type , NULL , NULL);
      if (file_type == ECL_UNIFIED_RESTART_FILE) {
	char * unif_restart_file   = argv[2];
	int report_nr1;
	int report_nr2;
	
	if (argc > 4) {
	  if ((util_sscanf_int( argv[3] , &report_nr1) && util_sscanf_int(argv[4] , &report_nr2))) {
	    section1 = load_restart_section( unif_restart_file , report_nr1 );
	    section2 = load_restart_section( unif_restart_file , report_nr2 );
	    arg_offset = 4;
	  } else
	    usage();
	} else
	  usage();
      } else if (file_type == ECL_RESTART_FILE) {
	if (argc > 3) {
	  char * restart_file1 = argv[2];
	  char * restart_file2 = argv[3];
	  section1 = ecl_file_fread_alloc( restart_file1 , true);
	  section2 = ecl_file_fread_alloc( restart_file2 , true);
	  arg_offset = 3;
	} else
	  usage();
      }
    } else
      usage();
    
    swat1 = ecl_kw_alloc_copy( ecl_file_iget_named_kw( section1 , "SWAT" , 0));
    swat2 = ecl_kw_alloc_copy( ecl_file_iget_named_kw( section2 , "SWAT" , 0));
    ecl_file_free( section1 );
    ecl_file_free( section2 );
  } 
  
  if (argc > arg_offset) {
    FILE * stream = util_fopen(argv[arg_offset + 1] , "r");
    int read_count;
    do {
      int i,j;
      read_count = fscanf(stream , "%d %d" , &i , &j);
      if (read_count == 2) {
  	int_vector_append(ilist , i);
  	int_vector_append(jlist , j);
      }
    } while (read_count == 2);
    fclose(stream);
  } else
    usage();


  if (argc > (arg_offset + 1))
    result_file = argv[arg_offset + 2];
  else
    usage();

  
  if (argc > (arg_offset + 2))
    surf_file = argv[arg_offset + 3];
  else
    surf_file = NULL;
  
  ecl_grid_get_dims( grid , &nx , &ny , &nz , &active_size );
  extract_contact(swat1 , swat2 , grid , ilist , jlist ,  result_file , surf_file);
  ecl_kw_free(swat1);
  ecl_kw_free(swat2);
  ecl_grid_free( grid );
  int_vector_free(ilist);
  int_vector_free(jlist);
}
