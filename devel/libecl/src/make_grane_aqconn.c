#include <stdbool.h>
#include <ecl_region.h>
#include <ecl_grid.h>
#include <ecl_file.h>
#include <ecl_kw.h>
#include <ecl_util.h>
#include <int_vector.h>
#include <parser.h>
#include <config.h> 
#include <string.h>



typedef struct options {
  double  min_swat;
  int     i1,i2;
  int     j1,j2;
  int     k1,k2;
  int     total_num_aq;
  char   *cell_fase;
  char   *aqudims_file;
  char   *aqucon_file;
  int     min_active;
  
  stringlist_type    * aqnames;
  int_vector_type    * aqnum;
  double_vector_type * aqtrans;
} options_type;


void make_aqudims_file(options_type * options , int_vector_type * aqcells , bool active) {
  FILE * stream = util_mkdir_fopen( options->aqudims_file , "w");
  if (active) {
    fprintf(stream , "AQUDIMS\n");
    fprintf(stream ,"  %d  %d / \n",options->total_num_aq , int_vector_size( aqcells ) * int_vector_size( options->aqnum ));
  }
  fclose( stream );
}



void make_aquconn_file(options_type * options , int_vector_type * aqcells , ecl_grid_type * ecl_grid , bool active) {
  FILE * stream = util_mkdir_fopen( options->aqucon_file , "w");
  int c;
  int iaq = 0;

  if (active) {
    fprintf(stream , "AQUCON\n");
    for (iaq = 0; iaq < int_vector_size( options->aqnum ); iaq++) {
      for (c = 0; c < int_vector_size( aqcells ); c++) {
        int i,j,k;
        ecl_grid_get_ijk1( ecl_grid , int_vector_iget( aqcells , c ) , &i , &j , &k);
        i++;
        j++;
        k++;
        fprintf(stream , "   %d  %4d  %4d  %4d  %4d   %4d %4d   \'%s\'  %12.6f /  %s\n", 
                int_vector_iget( options->aqnum , iaq) , i,i,j,j,k,k , options->cell_fase , 
                double_vector_iget( options->aqtrans , iaq) , 
                stringlist_iget(options->aqnames , iaq));
      }
    }
    fprintf(stream , "/\n");
  }
  fclose( stream );
}





options_type  * parse_alloc_options( const char * config_file , const ecl_grid_type * ecl_grid) {
  int nx,ny,nz;
  options_type     * options = util_malloc( sizeof * options , __func__);
  config_type      * config  = config_alloc();
  config_item_type * item;

  if (ecl_grid != NULL)
    ecl_grid_get_dims( ecl_grid , &nx , &ny , &nz , NULL);
  else {
    nx = 0;
    ny = 0;
    nz = 0;
  }
  
  {
    char * tmp = util_alloc_sprintf("%d" , nx);
    config_add_define(config , "<NX>" , tmp);
    config_add_define(config , "<nx>" , tmp);

    tmp = util_realloc_sprintf(tmp , "%d" , ny);
    config_add_define(config , "<NY>" , tmp);
    config_add_define(config , "<ny>" , tmp);

    tmp = util_realloc_sprintf(tmp , "%d" , nz);
    config_add_define(config , "<NZ>" , tmp);
    config_add_define(config , "<nz>" , tmp);
    
    free( tmp );
  }    
  
  item = config_add_item( config , "AQUIFER" , true , true );
  config_item_set_argc_minmax( item , 3 , 3 , (const config_item_types [3]) { CONFIG_INT , CONFIG_STRING , CONFIG_FLOAT });

  item = config_add_item( config , "MIN_SWAT" , true , false );
  config_item_set_argc_minmax( item , 1 , 1 , (const config_item_types [1]) { CONFIG_FLOAT });

  item = config_add_item( config , "BOX" , true , false );
  config_item_set_argc_minmax( item , 6 , 6 , (const config_item_types [6]) { CONFIG_INT , CONFIG_INT , CONFIG_INT , CONFIG_INT , CONFIG_INT , CONFIG_INT });

  item = config_add_item( config , "FASE" , true , false );
  config_item_set_argc_minmax( item , 1 , 1 , (const config_item_types [1]) { CONFIG_STRING });
  
  item = config_add_item( config , "MIN_ACTIVE" , true , false );
  config_item_set_argc_minmax( item , 1 , 1 , (const config_item_types [1]) { CONFIG_INT });

  item = config_add_item( config , "NUM_AQUIFER" , true , false );
  config_item_set_argc_minmax( item , 1 , 1 , (const config_item_types [1]) { CONFIG_INT });
  
  item = config_add_item( config , "AQUDIMS_FILE" , true , false );
  config_item_set_argc_minmax( item , 1 , 1 , (const config_item_types [1]) { CONFIG_STRING });

  item = config_add_item( config , "AQUCON_FILE" , true , false );
  config_item_set_argc_minmax( item , 1 , 1 , (const config_item_types [1]) { CONFIG_STRING });

  config_parse( config , config_file , "--" , NULL , NULL , NULL , false , true);

  options->min_swat     = config_iget_as_double( config , "MIN_SWAT" , 0 , 0);
  options->min_active   = config_iget_as_int( config , "MIN_ACTIVE" , 0 , 0);
  options->i1           = config_iget_as_int( config , "BOX" , 0 , 0) - 1; 
  options->i2           = config_iget_as_int( config , "BOX" , 0 , 1) - 1; 
  options->j1           = config_iget_as_int( config , "BOX" , 0 , 2) - 1; 
  options->j2           = config_iget_as_int( config , "BOX" , 0 , 3) - 1; 
  options->k1           = config_iget_as_int( config , "BOX" , 0 , 4) - 1; 
  options->k2           = config_iget_as_int( config , "BOX" , 0 , 5) - 1; 
  options->total_num_aq = config_iget_as_int( config , "NUM_AQUIFER" , 0 , 0);

  options->aqucon_file  = util_alloc_string_copy( config_iget( config , "AQUCON_FILE" , 0 , 0) );
  options->aqudims_file = util_alloc_string_copy( config_iget( config , "AQUDIMS_FILE", 0 , 0) );
  options->cell_fase    = util_alloc_string_copy( config_iget( config , "FASE" , 0 ,0 ) );
  
  options->aqnames = stringlist_alloc_new();
  options->aqnum   = int_vector_alloc(0,0);
  options->aqtrans = double_vector_alloc(0,0);
  {
    int iaq;
    for (iaq = 0; iaq < config_get_occurences( config , "AQUIFER" ); iaq++) {
      int_vector_append( options->aqnum ,        config_iget_as_int( config , "AQUIFER" , iaq , 0));
      stringlist_append_copy( options->aqnames , config_iget( config , "AQUIFER" , iaq , 1));
      double_vector_append( options->aqtrans ,   config_iget_as_double( config , "AQUIFER" , iaq , 2));
    }
  }
  
  return options;
}



void usage() {
  printf("Usage:\n\n   1: make_grane_aqconn  <GRID_FILE>  <Initial RESTART_FILE>   <config_file>\n");
  printf("             2: make_grane_aqconn  NOSIM   <config_file>\n\n");
  printf("The second form is for NOSIM use.\n");

  printf("Example config_file: \n");
  printf("-------------------- \n");
  printf("-- This is a configuration file for the program which makes a AQUCON\n");
  printf("-- include file for the GRANE model. The program works like follows:\n");
  printf("--\n");
  printf("--  1. For each i in [i1,i2] and k in [k1,k2] (defined with BOX below).\n");
  printf("--\n");
  printf("--  2. Start at j2 and scan down towards j1 until an active cell is\n");
  printf("--     found.\n");
  printf("--\n");
  printf("--  3. Verify that at least MIN_ACTIVE consecutive cells are active. If\n");
  printf("--     this can not be satisfied go back to 2 and continue scanning along\n");
  printf("--     the j direction.\n");
  printf("--\n");
  printf("--  4. OK - now we are at a cell which might be a an AQUIFER\n");
  printf("--     connection. Verify that the cell is in the water zone by comparing\n");
  printf("--     with MIN_SWAT.\n");
  printf("--\n");
  printf("--  5. OK - we have identified an AQUIFER cell.   \n");
  printf("\n");
  printf("\n");
  printf("-- The search is limited by the BOX defined below, i.e. the size\n");
  printf("-- arguments to BOX are: i1 i2 j1 j2 k1 k2. The box limits are inclusive,\n");
  printf("-- normal one-offset variables. When parsing the symbols <nx>, <ny> and\n");
  printf("-- <nz> are defined as total grid dimensions in x, y and z direction.\n");
  printf("BOX            1  30    166  <ny>   17  <nz>\n");
  printf("\n");
  printf("\n");
  printf("\n");
  printf("-- When we have found an active cell - how many more consecutive active\n");
  printf("-- cells do we require to mark this as an AQUIFER candidate.\n");
  printf("MIN_ACTIVE     3  \n");
  printf("\n");
  printf("\n");
  printf("-- What is the minimum water saturation to identify the cell as a water\n");
  printf("-- zone cell.\n");
  printf("MIN_SWAT       0.99\n");
  printf("\n");
  printf("\n");
  printf("\n");
  printf("-- When the program has (hopefully) run successfully it will create two\n");
  printf("-- include files. The AQUDIMS_FILE contains an AQUDIMS header and should\n");
  printf("-- be included in the RUNSPEC section. The AQUCON file contains ..\n");
  printf("AQUDIMS_FILE   AQUDIMS.INC\n");
  printf("AQUCON_FILE    AQUCON.INC\n");
  printf("\n");
  printf("\n");
  printf("-- This is the total number of numerical aquifers which must be set in\n");
  printf("-- the AQUDIMS header. This total can include AQUIFERS not defined here,\n");
  printf("-- it is therefor necessary(?) to input this explicitly (I think ...).\n");
  printf("NUM_AQUIFER   10\n");
  printf("\n");
  printf("\n");
  printf("FASE           J+    \n");
  printf("AQUIFER        3         Grane-Dummy-Heimdal    0.00015   \n");
  printf("AQUIFER        4         Grane-Balder           0.00150   \n");
  printf("\n");
  printf("-----------------------------------------------------------------\n");
  exit(1);
}






int main (int argc , char ** argv) {
  const char   * config_file;
  const char   * grid_file;
  const char   * restart_file;
  options_type * options;

  bool active = true;
  if (argc == 3) {
    if (strcmp(argv[1] , "NOSIM") == 0) {
      active = false;
      config_file = argv[2];
      options = parse_alloc_options( config_file , NULL );
      
      make_aquconn_file( options , NULL , NULL , false );
      make_aqudims_file( options , NULL , false);
      
    } else
      usage();
  } else if (argc == 4) {
    grid_file    = argv[1];
    restart_file = argv[2];
    config_file  = argv[3];
    
    {
      ecl_grid_type   * ecl_grid  = ecl_grid_alloc( grid_file );
      ecl_file_type   * restart;
      ecl_kw_type     * swat;
      int_vector_type * aqcells = int_vector_alloc(0,0);
      ecl_file_enum file_type;
      options = parse_alloc_options( config_file , ecl_grid);
  
    
      ecl_util_get_file_type( restart_file , &file_type , NULL , NULL);
      if (file_type == ECL_UNIFIED_RESTART_FILE)
        restart = ecl_file_fread_alloc_unrst_section( restart_file , 0);
      else
        restart = ecl_file_fread_alloc( restart_file );
      swat = ecl_file_iget_named_kw( restart , "SWAT" , 0 );
      
      {
        /* Inclusize zero offset limits. */
        const int jmin = options->j1;
        const int j2   = options->j2;
        const int i1   = options->i1;
        const int i2   = options->i2;
        const int k1   = options->k1;
        const int k2   = options->k2;
        
        int i,j,k;
        
        for (k=k1; k <= k2; k++) {
          for (i=i1; i <= i2; i++) {
            bool active;
            j    = j2;
            
            do {
              do {
                active = ecl_grid_cell_active3( ecl_grid , i,j,k);
                if (!active)
                  j--;
              } while (!active && j > jmin);
              
              if (active) {
                /* 
                   We found an active cell - check that minimum min_active
                   consecutive cells in j-direction are also active.
                */
                int offset = 1;
                do {
                  active = ecl_grid_cell_active3( ecl_grid , i , j - offset , k);
                  offset++;
                } while ( active && offset < options->min_active);
                if (!active)
                  j -= offset; /* Try again at this position */
              }
              
              if (active) {
                /* Check that the cell is in the water zone: */
                int active_index = ecl_grid_get_active_index3( ecl_grid , i , j , k);
                int global_index = ecl_grid_get_global_index3( ecl_grid , i , j , k);
                if (ecl_kw_iget_as_float( swat , active_index ) > options->min_swat)
                  int_vector_append( aqcells , global_index );
              }
            } while (!active && j > jmin);
            
          } 
        }
      }
      make_aquconn_file( options , aqcells , ecl_grid , active);
      make_aqudims_file( options , aqcells , active);
    
      ecl_file_free( restart );
      ecl_grid_free( ecl_grid );
    }
  } else
    usage();
}
