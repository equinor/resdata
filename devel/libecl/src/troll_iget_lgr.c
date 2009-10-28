#include <stdlib.h>
#include <stdio.h>
#include <ecl_grid.h>
#include <ecl_file.h>
#include <parser.h>
#include <util.h>


/*
  1) A-08 C     1628 m TVD MSL (Fensfjord)           ---> ingen LGR ; grov=(27 48 14)   cell nr: 39501   depth: 1626.3 m
  
  2) A-08 E     1493 m TVD MSL (Nedre Sognefjord 2C) ---> grov=(27 48 10)  LGR=’TROLLA2’    fin=(10,10,3)     cell nr: 62922   depth: 1492.9 m
  
  3) A-08 F     1458 m TVD MSL (Sognefjord 4M)       ---> grov=(27 48 8)   LGR=’TROLLA’     fin=(10,10,6)     cell nr: 60402   depth: 1451.0 m
     Not used:                                                                              fin=(10,10,7)     cell nr: 60906   depth: 1464.5 m  
										     
  4) A-08 G     1413 m TVD MSL (Sognefjord 5C)       ---> grov=(27 48 4)   LGR=’TROLLA’     fin=(10,10,3)     cell nr: 58890   depth: 1416.1 m 

  5) A-08 H     1379 m TVD MSL (Sognefjord 6C7)      ---> grov=(27 48 2)   LGR=’TROLLA’     fin=(10,10,1)     cell nr: 57882   depth: 1391.1 m
*/





int main(int argc , char ** argv) {
  const bool    fmt_file       = false;
  const char  * grid_file      = argv[1];
  const char  * ecl_base       = argv[2];
  const char  * config_file    = argv[3];
  const char  * OK_file        = argv[4];
  const int     step_offset    = 5;          
  const char ** step_list      = (const char ** )&argv[step_offset];


  parser_type  * parser       = parser_alloc(" \n\r" , "\"\'" , NULL , NULL , "--" , "\n");
  stringlist_type * tokens    = parser_tokenize_file( parser  	   , config_file , true);
  ecl_grid_type * ecl_grid    = ecl_grid_alloc( grid_file );
  
  for (int i=0; i < (argc - step_offset); i++) {
    int step;
    if (util_sscanf_int( step_list[i] , &step)) {
      char * restart_file = ecl_util_alloc_exfilename( NULL , ecl_base , ECL_RESTART_FILE , fmt_file , step);
      if (restart_file != NULL) {
        ecl_file_type * ecl_file = ecl_file_fread_alloc( restart_file );
        printf("Extracting data from:%s \n",restart_file );
        for (int i=0; i < stringlist_get_size( tokens ); i += 3) {
          const char * lgr_name    = stringlist_iget( tokens , i );
          const char * ijk_string  = stringlist_iget( tokens , i + 1);
          const char * outfile_fmt = stringlist_iget( tokens , i + 2);
          char *       outfile     = util_alloc_sprintf( outfile_fmt , step );
          {
            ecl_grid_type * lgr 	   = ecl_grid_get_lgr( ecl_grid , lgr_name );
            int grid_nr         	   = ecl_grid_get_grid_nr( lgr );
            ecl_kw_type  * pressure_kw = ecl_file_iget_named_kw( ecl_file , "PRESSURE" , grid_nr );
            int i,j,k;
            FILE * stream              = util_fopen( outfile , "w" );
            if (sscanf( ijk_string , "%d,%d,%d" , &i , &j , &k ) != 3) 
              util_exit("Failed to read i,j,k from: %s \n",ijk_string);
            
            /* Only ONE measurement for each file. */
            fprintf(stream , "%g\n", ecl_kw_iget_float( pressure_kw , ecl_grid_get_active_index3( lgr , i-1 , j-1 , k-1)));
            fclose( stream );
          }
          free( outfile );
        }
        ecl_file_free( ecl_file );
      } 
      free(restart_file);
    } else 
      util_exit("Failed to interpret %s as integer\n", step_list[i]);
  }
    
  parser_free( parser );
  stringlist_free( tokens );
  ecl_grid_free( ecl_grid );
  {
    FILE * stream = util_fopen(OK_file , "w");
    fprintf(stream , "All OK with pressure measurements\n");
    fclose(stream);
  }
}


   
