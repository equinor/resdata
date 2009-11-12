#include <stdlib.h>
#include <stdio.h>
#include <ecl_grid.h>
#include <ecl_file.h>
#include <parser.h>
#include <util.h>
#include <config.h>

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






static double find_contact(const ecl_grid_type * lgr , const ecl_file_type * restart_file , int i , int j , 
                           const ecl_kw_type * satnum_kw , 
                           const double * sgcr , 
                           const double * swco ,
			   double sw0  , 
                           double initial_contact) {
  double contact_depth              = -1;        
  const ecl_grid_type * global_grid = ecl_grid_get_global_grid( lgr );
  int grid_nr                       = ecl_grid_get_grid_nr( lgr );
  ecl_kw_type * swat_kw             = ecl_file_iget_named_kw( restart_file , "SWAT" , grid_nr );
  int nz                            = ecl_grid_get_nz( lgr );
  int k;
  if (global_grid == NULL)
    global_grid = lgr;
  
  
  /* Starting at the bottom and going up. */
  for (k=nz - 1; k >= 0; k--) {
    bool   cell_contains_contact = false;
    int    index                 = ecl_grid_get_active_index3( lgr , i , j , k);
    double swat                  = ecl_kw_iget_as_double( swat_kw , index );
    double top    	         = ecl_grid_get_top3(lgr , i , j , k);
    double bottom 	         = ecl_grid_get_bottom3(lgr , i , j , k);
    double cell_thickness        = (bottom - top);
    double swat_upper_limit      = 0.999 * sw0;
    int    gactive_index , satnum;
    {
      int    global_index = ecl_grid_get_parent_cell3( lgr , i , j , k );
      gactive_index       = ecl_grid_get_active_index1( global_grid , global_index );
    }
    satnum = ecl_kw_iget_int( satnum_kw , gactive_index ) - 1;
    
    if (initial_contact < 0) {
      /* We are looking for the inital contact - that is relatively easier. */
      if (swat < swat_upper_limit) 
        cell_contains_contact = true;
    } else {
      /*
        Detecting the cell with the contact must be based on a
        criteria where the initial contact level in the cell is NOT
        considered. This is because the following: 

           That the cell which contained the initial contact should
           have a higher water saturation than (1 - sgcr) at full
           flooding is not correctly accounted for by ECLIPSE. The
           cell will not reach higher water saturation than (1 -
           sgcr), that is therefor the upper limit we must consider.
           
           In the actual calculation of contact depth further down the
        thickness of the initial gas layer, in the cell, is
        "correctly" accounted for.
      */
      swat_upper_limit = 0.999* (1 - sgcr[satnum]);   /* Fully water flooded */
      if (swat < swat_upper_limit) 
        cell_contains_contact = true;
    }
    
    if (cell_contains_contact) {
      /* 
         OK - we have found the cell:
         
         Going from the lgr cell (i,j,k) to the corresponding global cell, 
         this is to look up the correct values for sgcr and swco. 
      */
      {
        
        if (initial_contact < 0) {
          double h;
          /*
            Equation to solve inside cell:
            
            h * sw0 + (cell_thickness - h) * swco = swat * cell_thickness   =>  h = cell_thickness * swco / (swco + swat - 1)
            
          */
          
          h = cell_thickness * (swat - swco[satnum]) / (sw0 - swco[satnum]);
          contact_depth = bottom - h;
          break;
        } else {
          double initial_thickness = 0;
          double delta_h;
          
          /* The initial contact was in this cell. */
          if (bottom > initial_contact) 
            initial_thickness = bottom - initial_contact;
          /*
            Equation to solve:
            
            initial_thickness * Sw0 + delta_h * (1 - sgcr) + (cell_thickness - (delta_h + initial_thickness)) * swco = swat * cell_thickness
            
            
            ==>  delta_h [(1 - sgcr) - swco] = cell_thickness * swat - initial_thickness * Sw0 - swco * (cell_thickenss - initial_thickness)
          
          */
     
          delta_h = (cell_thickness * swat - initial_thickness * sw0 - (cell_thickness - initial_thickness) * swco[satnum]) / ( 1- sgcr[satnum] - swco[satnum] );
          contact_depth = bottom - delta_h - initial_thickness;
          break;
        }
      }
    }
  }
  return contact_depth;
}



static void parse_config( const char * config_file , char ** bottom_lgr , char ** top_lgr , int *i , int *j , double ** __sgcr, double ** __swco) {
  int k;
  int num_relperm_regions = 26;
  config_type * config = config_alloc();
  config_add_key_value( config , "BOTTOM_LGR" , true , CONFIG_STRING );
  config_add_key_value( config , "TOP_LGR"    , true , CONFIG_STRING );
  config_add_key_value( config , "I"          , true , CONFIG_INT );
  config_add_key_value( config , "J"          , true , CONFIG_INT );
  {
    config_item_type  * item;
    config_item_types * typemap = util_malloc( num_relperm_regions * sizeof * typemap , __func__);
    
    for (k = 0; k < num_relperm_regions; k++)
      typemap[k] = CONFIG_FLOAT;
    
    item = config_add_item( config , "SGCR" , true , false );   
    config_item_set_argc_minmax( item , num_relperm_regions , num_relperm_regions , typemap);
    
    item = config_add_item( config , "SWCO" , true , false );
    config_item_set_argc_minmax( item , num_relperm_regions , num_relperm_regions , typemap);
    free(typemap);
  }
  
  config_parse( config , config_file , "--" , NULL , NULL , NULL , false , true );
  
  *bottom_lgr = util_alloc_string_copy( config_get_value( config , "BOTTOM_LGR" ));
  *top_lgr    = util_alloc_string_copy( config_get_value( config , "TOP_LGR" ));
  *i          = config_get_value_as_int( config , "I" );
  *j          = config_get_value_as_int( config , "J" );
  
  {
    double * sgcr = util_malloc( num_relperm_regions * sizeof * sgcr , __func__ );
    double * swco = util_malloc( num_relperm_regions * sizeof * swco , __func__ );
    for (k=0; k < num_relperm_regions; k++) {
      sgcr[k] = config_iget_as_double( config , "SGCR" , 0 , k );
      swco[k] = config_iget_as_double( config , "SWCO" , 0 , k );
    }
    *__sgcr = sgcr;
    *__swco = swco;
  }
  
  config_free( config );
}






int main(int argc , char ** argv) {
  char * bottom_lgr;
  char * top_lgr;
  int i;
  int j;
  double * sgcr;
  double * swco;
  double initial_contact;


  
  const bool   fmt_file       = false;
  {
    const char * grid_file      = argv[1];
    const char * restart_file1  = argv[2];
    const char * init_file_name = argv[3];
    const char * ecl_base       = argv[4];
    const char * config_file    = argv[5]; 
    const char * OK_file        = argv[6];
    const int     step_offset   = 7;          
    const char ** step_list     = (const char ** )&argv[step_offset];

    
    ecl_grid_type * ecl_grid    = ecl_grid_alloc( grid_file );
    ecl_file_type * init_file   = ecl_file_fread_alloc( init_file_name );
    ecl_kw_type   * satnum_kw   = ecl_file_iget_named_kw( init_file , "SATNUM" , 0 );
    parse_config( config_file , &bottom_lgr , &top_lgr , &i , &j , &sgcr , &swco );
    
    
    {
      ecl_grid_type * lgr;
      ecl_file_type * ecl_file = ecl_file_fread_alloc( restart_file1 );            

      lgr   = ecl_grid_get_lgr( ecl_grid , bottom_lgr );
      initial_contact = find_contact(lgr , ecl_file , i , j , satnum_kw , sgcr , swco , 1.0 , -1);
      if (initial_contact < 0) {
        lgr             = ecl_grid_get_lgr( ecl_grid , top_lgr );
        initial_contact = find_contact(lgr , ecl_file , i , j , satnum_kw , sgcr , swco , 1.0 , -1);
      }
      if (initial_contact < 0)
        util_exit("Failed to locate contact - giving up.\n");
      ecl_file_free( ecl_file );
    }
    printf("Initial contact:%g \n",initial_contact);

    for (int istep =0; istep < (argc - step_offset); istep++) {
      int step;
      if (util_sscanf_int( step_list[istep] , &step)) {
	char * restart_file = ecl_util_alloc_filename( NULL , ecl_base , ECL_RESTART_FILE , fmt_file , step);
        if (util_file_exists( restart_file )) {
	  ecl_file_type * ecl_file = ecl_file_fread_alloc( restart_file );
          
          {
	    ecl_grid_type * lgr;
	    double contact;
            
	    lgr   = ecl_grid_get_lgr( ecl_grid , bottom_lgr );
	    contact = find_contact(lgr , ecl_file , i , j , satnum_kw , sgcr , swco , 1.0 , initial_contact);
	    if (contact < 0) {
	      lgr   = ecl_grid_get_lgr( ecl_grid , top_lgr );
	      contact = find_contact(lgr , ecl_file , i , j , satnum_kw , sgcr , swco , 1.0 , initial_contact);
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


   
