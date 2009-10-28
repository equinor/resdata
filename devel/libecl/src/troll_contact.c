#include <stdlib.h>
#include <stdio.h>
#include <ecl_grid.h>
#include <ecl_file.h>
#include <parser.h>
#include <util.h>


/*
  1. Below initial contact: SWAT == 1.
  2. Above current contact: SWAT == SWCO
  3. Between the two:       SWAT == 1 - SGCR


  |  
  |  SWAT = SWCO
  |  
  |-------------- Current contact 
  |  
  |  SWAT == 1 - SGCR 
  |  
  |-------------- Initial contact
  |  
  |  SWAT == 1
  |  

*/



/* 
   
TROLLA  27,13,k
TROLLA2 27,13,k
   
*/


static void swat_column(const ecl_grid_type * lgr , 
                        const ecl_file_type * restart_file , int i , int j ) {
  
  int grid_nr              = ecl_grid_get_grid_nr( lgr );
  ecl_kw_type * swat_kw    = ecl_file_iget_named_kw( restart_file , "SWAT" , grid_nr );
  int nz                   = ecl_grid_get_nz( lgr );
  int k;
  
  for (k=0; k < nz; k++) {
    int    index = ecl_grid_get_active_index3( lgr , i , j , k);
    double swat  = ecl_kw_iget_as_double( swat_kw , index );
    printf("%10.7f %d: [%g,%g]\n",swat,k,ecl_grid_get_bottom3(lgr , i , j , k) , ecl_grid_get_top3(lgr , i , j , k));
  }
}


static double find_contact(const ecl_grid_type * lgr , const ecl_file_type * restart_file , int i , int j , 
                           double sgcr , 
			   double swco , 
			   double sw0  , 
                           double initial_contact) {
  const double water_limit = 0.80 * sw0;
  double contact_depth     = -1;        
  int grid_nr              = ecl_grid_get_grid_nr( lgr );
  ecl_kw_type * swat_kw    = ecl_file_iget_named_kw( restart_file , "SWAT" , grid_nr );
  int nz                   = ecl_grid_get_nz( lgr );
  int k;

  
  /* Starting at the bottom and going up. */
  for (k=nz - 1; k >= 0; k--) {
    int    index = ecl_grid_get_active_index3( lgr , i , j , k);
    double swat  = ecl_kw_iget_as_double( swat_kw , index );
    printf("%10.7f %d: [%g,%g]\n",swat,k,ecl_grid_get_bottom3(lgr , i , j , k) , ecl_grid_get_top3(lgr , i , j , k));
    if (swat < water_limit) {
      /*OK - we have found the cell */
      double top    	       = ecl_grid_get_top3(lgr , i , j , k);
      double bottom 	       = ecl_grid_get_bottom3(lgr , i , j , k);
      double cell_thickness    = (bottom - top);
      printf("Contact located in cell: %s:%d   [%g , %g] \n",ecl_grid_get_name( lgr ) , k , bottom , top);

      if (initial_contact < 0) {
        double h;
        /*
          Equation to solve inside cell:
          
          h *sw0 + (cell_thickness - h) * swco = swat * cell_thickness   =>  h = cell_thickness * swco / (swco + swat - 1)
          
        */
        
        h = cell_thickness * (swat - swco) / (sw0 - swco);
        contact_depth = bottom - h;
        printf("Initial (cell) thickness:%g cell_thickness:%g \n",h , cell_thickness);

        break;
      } else {
        double initial_thickness = 0;
        double delta_h;
        
        /* The initial contact was in this cell. */
        if (bottom > initial_contact) 
          initial_thickness = bottom - initial_contact;
        printf("initial_thickness:%g \n",initial_thickness);
        
        /*
          Equation to solve:
          
          initial_thickness * Sw0 + delta_h * (1 - sqgcr) + (cell_thickness - (delta_h + initial_thickness)) * swco = swat * cell_thickness
          
          
          ==>  delta_h [(1 - sgcr) - swco] = cell_thickness * swat - initial_thickness * Sw0 - swco * (cell_thickenss - initial_thickness)
          
        */
        delta_h = (cell_thickness * swat - initial_thickness * sw0 - (cell_thickness - initial_thickness) * swco) / ( 1- sgcr - swco );
        contact_depth = bottom - delta_h - initial_thickness;
        break;
      }
    }
  }
  return contact_depth;
}



static void parse_config( const char * config_file , char ** bottom_lgr , char ** top_lgr , int *i , int *j , double * sgcr, double *swco) {
  parser_type * parser = parser_alloc(" \n\r\t" , NULL , NULL , NULL , "--" , "\n");
  stringlist_type * tokens = parser_tokenize_file( parser , config_file , false);

  if (stringlist_get_size( tokens ) == 7) {
    *bottom_lgr = util_alloc_string_copy( stringlist_iget( tokens , 0));
    *top_lgr    = util_alloc_string_copy( stringlist_iget( tokens , 1));
    {
      bool OK = true;
      OK = (OK && util_sscanf_int(stringlist_iget(tokens , 2)    , i));
      OK = (OK && util_sscanf_int(stringlist_iget(tokens , 3)    , j));
      OK = (OK && util_sscanf_double(stringlist_iget(tokens , 4) , sgcr));
      OK = (OK && util_sscanf_double(stringlist_iget(tokens , 5) , swco));

      if (!OK) 
	util_exit("Failed to parse config file:%s correctly\n",config_file);
    }
  } else
    util_exit("Failed to parse config file:%s correctly\n",config_file);
  
  stringlist_free( tokens );
  parser_free( parser );
}






int main(int argc , char ** argv) {
  char * bottom_lgr;
  char * top_lgr;
  int i;
  int j;
  double sgcr;
  double swco;
  double initial_contact;


  
  const bool   fmt_file       = false;
  {
    const char * grid_file      = argv[1];
    const char * restart_file1  = argv[2];
    const char * ecl_base       = argv[3];
    const char * config_file    = argv[4]; 
    const char * OK_file        = argv[5];
    const int     step_offset   = 6;          
    const char ** step_list     = (const char ** )&argv[step_offset];

    parse_config( config_file , &bottom_lgr , &top_lgr , &i , &j , &sgcr , &swco );
    ecl_grid_type * ecl_grid    = ecl_grid_alloc( grid_file );
    
                                                        

    {
      ecl_grid_type * lgr;
      ecl_file_type * ecl_file = ecl_file_fread_alloc( restart_file1 );            

      printf("-----------------------------------------------------------------\n");
      swat_column( ecl_grid_get_lgr(ecl_grid , top_lgr )    , ecl_file , i , j );
      swat_column( ecl_grid_get_lgr(ecl_grid , bottom_lgr ) , ecl_file , i , j );
      printf("-----------------------------------------------------------------\n");

      lgr   = ecl_grid_get_lgr( ecl_grid , bottom_lgr );
      initial_contact = find_contact(lgr , ecl_file , i , j , sgcr , swco , 1.0 , -1);
      if (initial_contact < 0) {
        lgr             = ecl_grid_get_lgr( ecl_grid , top_lgr );
        initial_contact = find_contact(lgr , ecl_file , i , j , sgcr , swco , 1.0 , -1);
      }
      if (initial_contact < 0)
        util_exit("Failed to locate contact - giving up.\n");
      ecl_file_free( ecl_file );
    }
    printf("Initial contact:%g \n",initial_contact);

    for (int istep =0; istep < (argc - step_offset); istep++) {
      int step;
      if (util_sscanf_int( step_list[istep] , &step)) {
	char * restart_file = ecl_util_alloc_exfilename( NULL , ecl_base , ECL_RESTART_FILE , fmt_file , step);
	if (restart_file != NULL) {
	  ecl_file_type * ecl_file = ecl_file_fread_alloc( restart_file );
          
          printf("-----------------------------------------------------------------\n");
          swat_column( ecl_grid_get_lgr( ecl_grid , top_lgr )    , ecl_file , i , j );
          swat_column( ecl_grid_get_lgr( ecl_grid , bottom_lgr ) , ecl_file , i , j );
          printf("-----------------------------------------------------------------\n");
          

	  {
	    ecl_grid_type * lgr;
	    double contact;
            
	    lgr   = ecl_grid_get_lgr( ecl_grid , bottom_lgr );
	    contact = find_contact(lgr , ecl_file , i , j , sgcr , swco , 1.0 , initial_contact);
	    if (contact < 0) {
	      lgr   = ecl_grid_get_lgr( ecl_grid , top_lgr );
	      contact = find_contact(lgr , ecl_file , i , j , sgcr , swco , 1.0 , initial_contact);
	    }
	    if (contact < 0)
	      util_exit("Failed to locate contact - giving up.\n");
	    
            printf("Second contact:%g \n",contact);
            
	    {
	      char * out_file = util_alloc_sprintf("contact_change_%04d" , step);
	      FILE * stream   = util_fopen( out_file , "w");
	      fprintf(stream , "%g\n",initial_contact - contact);
	      fclose(stream);
	      free(out_file);
	    }
	  }
	  free(restart_file);
	  ecl_file_free( ecl_file );
	} else
          util_exit("troll_contact: could not locate restart_file:%s \n", restart_file);
	/*
	  else: The restart file did not exist - fair enough.
	*/
      } else 
        util_exit("Failed to interpret %s as integer \n",step_list[i]);
    } 
    {
      FILE * stream = util_fopen(OK_file , "w");
      fprintf(stream , "All OK with contact measurements\n");
      fclose(stream);
    }
  }
}


   
