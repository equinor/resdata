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

/*
  At svn:2031 the program is slightly rewritten:

  o Removed the extract_contact_peak() function at svn 2031
  o The enkf mode is removed.
  o The file with indices is the last command-line argument, and optional.
  
*/
  



void extract_contact(const ecl_kw_type   * swat1    , 
		     const ecl_kw_type   * swat2    , 
		     const ecl_grid_type * ecl_grid , 
		     const int_vector_type * ilist  , 
		     const int_vector_type * jlist  , 
		     const char * surf_file) {

  int index,nz,active_size,nx,ny;
  double * sw1 , * sw2;
  FILE * surf_stream = util_fopen( surf_file , "w");
  
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
  
      if (owc > 0)
	fprintf(surf_stream   , "%18.6f  %18.6f  %12.6f  \n",xpos,ypos,owc);

    }
  }

  fclose(surf_stream);
  free( sw1 );
  free( sw2 );
}




static ecl_file_type * load_restart_section( const char * unif_restart_file , int report_nr) {

  ecl_file_type * section;
  printf("Loading report step %04d from: %s .....", report_nr , unif_restart_file); fflush(stdout);
  section = ecl_file_fread_alloc_unrst_section(  unif_restart_file , report_nr );
  if (section == NULL)
    util_exit("exit: failed to load report_step:%d from file:%s.\n", report_nr , unif_restart_file);
  printf("\n");
  return section;

}


static void load_ij(const ecl_grid_type * grid , const char * ij_file, int_vector_type * ilist , int_vector_type * jlist) {
  int i,j;
  if (ij_file != NULL) {
    FILE * stream = util_fopen(ij_file , "r");
    int read_count;
    printf("Loading i,j from file:%s...\n", ij_file);
    do {
      read_count = fscanf(stream , "%d %d" , &i , &j);
      if (read_count == 2) {
	int_vector_append(ilist , i);
	int_vector_append(jlist , j);
      }
    } while (read_count == 2);
  } else {
    /* 
       The user has not specified a file with (i,j) pairs, we just
       scan the whold god damn grid.
    */
    int nx = ecl_grid_get_nx( grid );
    int ny = ecl_grid_get_ny( grid );
    printf("Using i,j from the whole field\n");
    for (i=0; i < nx; i++) {
      for (j=0; j < ny; j++) {
	int_vector_append(ilist , i);
	int_vector_append(jlist , j);
      }
    }
  }
}


static void usage() {
  printf("*****************************************************************\n");
  printf("The extract_owc.x program needs the following input on the command-line\n");
  printf("\n");
  printf("   1. The name of an ECPLISE GRID/EGRID file.\n");
  printf("\n");
  printf("   2. The name of ECLIPSE unified restart file and the report steps for\n");
  printf("      base and monitor surveys respectively.\n");
  printf("\n");
  printf("         or\n");
  printf("\n");
  printf("   2. The name of two non-unified ECLIPSE restart files from the base\n");
  printf("      and monitor times respectively.\n");
  printf("\n");
  printf("   3. [Output:] The name of a file where columns \"x y owc\" are stored, \n");
  printf("      this can be loaded into e.g. RMS to see the new estimated contact.\n");
  printf("\n");
  printf("  [4]. The name of a file with i,j index pairs for the region you are\n");
  printf("       interested in. This argument is _optional_,  if not given, the \n");
  printf("       program will determine the OWC for the whole field.\n\n");
  printf("=================================================================\n");
  printf("\nExample1:\n");
  printf("\n");
  printf("  bash%% extract_contact.x  ECLIPSE.EGRID   ECLIPSE.UNRST   0  100   OWC_field.xyz \n");
  printf("\n");

  printf("This will load restart information from the unified restart file\n");
  printf("'ECLIPSE.UNRST', the program will use SWAT results from the report\n");
  printf("steps 0 and 100. The resulting x,y,z surface will be stored in the\n");
  printf("file 'OWC_field.xyz'. No file with (i,j) index pairs is given, hence\n");
  printf("the program will calculate the OWC for the whole field.\n");
  printf("\n");
  printf("\n");
  printf("-----------------------------------------------------------------\n");
  printf("\nExample2:\n");
  printf("\n");
  printf("  bash%% extract_contact.x  ECLIPSE.GRID   ECLIPSE.X0000   ECLIPSE.X0075 OWC_C.xyz  IJ_C\n");
  printf("\n");

  printf("In this example the restart information is loaded from non-unified\n");
  printf("restart files, 'ECLIPSE.X0000' and 'ECLIPSE.X0175', since the\n");
  printf("report step is part of the filename in this case it is not\n");
  printf("necessary to give that separately. The resulting x,y,z surface\n");
  printf("will be stored in a file 'OWC_C.xyz'. Finally a file 'IJ_C' with\n");
  printf("(i,j) indices for the region of interest is given. Since such a\n");
  printf("file is given only part of the field will be considered.\n");
  
  printf("*****************************************************************\n");
  exit(1);
}



int main (int argc , char ** argv) {
  char * surf_file = NULL;
  int_vector_type * ilist    = int_vector_alloc( 0 , -1 );
  int_vector_type * jlist    = int_vector_alloc( 0 , -1 );
  ecl_grid_type * grid         = NULL;

  int next_iarg = -1;
  
  ecl_kw_type 	* swat1 = NULL;
  ecl_kw_type 	* swat2 = NULL;
  
  
  /* Loading the grid */
  if (argc > 1) {
    printf("Loading grid file:%s ......" , argv[1]); fflush(stdout);
    grid = ecl_grid_alloc( argv[1] );    
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
	/* Loading from a unifed restart file. */
	char * unif_restart_file = argv[2];
	int report_nr1;
	int report_nr2;
	
	if (argc > 4) {
	  if ((util_sscanf_int( argv[3] , &report_nr1) && util_sscanf_int(argv[4] , &report_nr2))) {
	    section1  = load_restart_section( unif_restart_file , report_nr1 );
	    section2  = load_restart_section( unif_restart_file , report_nr2 );
	    next_iarg = 5;
	  } else
	    usage();
	} else
	  usage();
      } else if (file_type == ECL_RESTART_FILE) {
	/* Loading from two non-unified restart files. */
	if (argc > 3) {
	  char * restart_file1 = argv[2];
	  char * restart_file2 = argv[3];
	  printf("Loading restart file: %s...\n", restart_file1);
	  section1 = ecl_file_fread_alloc( restart_file1 );
	  printf("Loading restart file: %s...\n", restart_file2);
	  section2 = ecl_file_fread_alloc( restart_file2 );
	  next_iarg = 4;
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
  /* 
     The filename for the resulting surface.
  */
  if (argc > (next_iarg))
    surf_file = argv[next_iarg];
  else
    usage();
  
  {
    char * index_file = NULL;
    if (argc > (next_iarg + 1))
      index_file = argv[next_iarg + 1];
    load_ij(grid , index_file , ilist , jlist);
  }
  
  printf("Writing surface file: %s" , surf_file); fflush(stdout);
  extract_contact(swat1 , swat2 , grid , ilist , jlist ,  surf_file);
  printf(" \n");
  ecl_kw_free(swat1);
  ecl_kw_free(swat2);
  ecl_grid_free( grid );
  int_vector_free(ilist);
  int_vector_free(jlist);
}
