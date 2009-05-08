#include <util.h>
#include <hash.h>
#include <stdio.h>
#include <string.h>
#include <ecl_file.h>
#include <ecl_util.h>
#include <vector.h>
#include <ecl_grid.h>
#include <math.h>

#define WATER 1
#define GAS   2
#define OIL   4


/*****************************************************************/



void truncate_saturation(float * value) {
  util_apply_float_limits( value , 0.0 , 1.0);
}


bool has_phase( int phase_sum , int phase) {
  if ((phase_sum & phase) == 0)
    return false;
  else
    return true;
}


const float * safe_get_float_ptr( const ecl_kw_type * ecl_kw , const float * alternative) {
  if (ecl_kw != NULL)
    return ecl_kw_get_float_ptr(ecl_kw);
  else
    return alternative;
}




/*****************************************************************/

void print_usage() {
  printf("***************************************************************************\n");
  printf("  This program is used to calculate the change in graviational response \n");
  printf("  between two timesteps in an eclipse simulation.\n");
  printf("\n");
  printf(" USAGE: ./run_gravity.x [-h] base_name restart_time1 restart_time2 station_position_file\n");
  printf("\n");
  printf(" NOTE: The program needs a *.GRID, *.INIT, as well as the two restart files.\n");
  printf(" NOTE: The reservoir pore volume needs to be reported in the restart file: \n");
  printf(" NOTE:  Eclipse kw RPORV in RPTRST\n");
  
  printf("***************************************************************************\n");
}

typedef struct {
  double utm_x; 
  double utm_y; 
  double depth; 
  double grav_diff;
} grav_station_type;


grav_station_type * grav_station_alloc(double x, double y, double d){
  grav_station_type * s = util_malloc(sizeof*s, __func__);
  s->utm_x = x;
  s->utm_y = y;
  s->depth = d;
  s->grav_diff = 0.0;
  return s;
}

void grav_diff_update(grav_station_type * g_s, double inc){
  //printf("Updateing\n");
  g_s->grav_diff = g_s->grav_diff + inc;
}



/*****************************************************************/

int main(int argc , char ** argv) {
  
  if(argc > 1){
    if(strcmp(argv[1], "-h") == 0){
      print_usage();
      exit(1);
    }
  }
  if(argc < 2){
    print_usage();
    exit(1);
  }
  else{
    int model_phases;
    int file_phases;

    const char * ecl_base     = argv[1];    
    const char * time1        = argv[2];    
    const char * time2        = argv[3];    
    const char * station_file = argv[4];    
    
    // Check if all the relevant files exist
    
    vector_type * grav_stations = vector_alloc_new();
    char * grid_filename     	= ecl_util_alloc_exfilename( NULL , ecl_base , ECL_GRID_FILE , false , -1);
    char * init_filename     	= ecl_util_alloc_exfilename( NULL , ecl_base , ECL_INIT_FILE , false , -1);
    char * restart1_filename;
    char * restart2_filename;
    
    {
      int restart1,restart2;
      if (!(util_sscanf_int( time1 , &restart1) && util_sscanf_int( time2 , &restart2 )))
	printf("Sorry failed to interpret %s and %s as integers\n", time1 , time2);

      restart1_filename = ecl_util_alloc_exfilename( NULL , ecl_base , ECL_RESTART_FILE , false , restart1);
      restart2_filename = ecl_util_alloc_exfilename( NULL , ecl_base , ECL_RESTART_FILE , false , restart2);
    }

    // station_file
    if (!(util_file_exists(station_file))) {
      printf("Sorry, file:, %s does not exist", station_file);
      exit(1);
    }
    
    {
      FILE * stream = util_fopen(station_file , "r");
      bool at_eof = false;
      while(!(at_eof)) {
	double x,y,d;
	int fscanf_return = fscanf(stream, "%lg%lg%lg", &x,&y,&d);
	if(fscanf_return ==3){
	  grav_station_type * g = grav_station_alloc(x,y,d);
	  vector_append_owned_ref(grav_stations, g, free);
	}
	//else if(fscanf_return == 0) {
	//  at_eof = true;
	//}
	else{
	  at_eof = true;
	  //util_abort("%s: something funky - only found %d numbers", __func__, fscanf_return);
	}
      }
      fclose(stream);
    }
    
    
    // Allocate the files 
    ecl_file_type * init_file     = ecl_file_fread_alloc(init_filename , true);
    ecl_grid_type * grid_file     = ecl_grid_alloc(grid_filename , true);
    ecl_file_type * restart1_file = ecl_file_fread_alloc(restart1_filename , true);
    ecl_file_type * restart2_file = ecl_file_fread_alloc(restart2_filename , true);

    int nx,ny,nz,nactive;    
    ecl_grid_get_dims( grid_file , &nx , &ny , &nz , &nactive);
    printf("NACTVE CELLS IS: %d %d %d %d\n", nactive, nx, ny, nz);

        
    // Get the relevant kws and vectors
    // RPORV
    ecl_kw_type * rporv1_kw  ;
    ecl_kw_type * rporv2_kw ;
    
    model_phases = 0;
    if (ecl_file_has_kw(restart1_file , "OIL_DEN"))
      model_phases += OIL;			  	      

    if (ecl_file_has_kw(restart1_file , "WAT_DEN"))
      model_phases += WATER;			  	      

    if (ecl_file_has_kw(restart1_file , "GAS_DEN"))
      model_phases += GAS;

    /** We assume the restart file NEVER has SOIL information */
    file_phases = 0;
    if (ecl_file_has_kw(restart1_file , "SWAT"))
      file_phases += WATER;
    if (ecl_file_has_kw(restart1_file , "SGAS"))
      file_phases += GAS;
    if (ecl_file_has_kw(restart1_file , "OIL"))
      file_phases += OIL;


    /* Consiency check */
    {
      /**
	 The following assumptions are made:
	 
	 1. All restart files should have water, i.e. the SWAT keyword. 
	 2. All phases present in the restart file should also be present as densities.
	 3. The restart files can never contain oil saturation.
	 
      */
      if (! has_phase( file_phases , WATER ) )
	util_exit("Could not locate SWAT keyword in restart files\n");
      
      if ( has_phase( file_phases , OIL ))
	util_exit("Can not handle restart files with SOIL keyword\n"); 
      
      if (! has_phase( model_phases , WATER ) )
	util_exit("Could not locate WAT_DEN keyword in restart files\n");      
      
      if ( has_phase( file_phases , GAS )) {
	/** Restart file has both water and gas - means we need all three densities. */
	if (! (has_phase(model_phases , GAS) && has_phase(model_phases , OIL)))
	  util_exit("Could not find GAS_DEN and OIL_DEN keywords in restart files\n");
      } else {
	/* This is (water + oil) or (water + gas) system. We enforce one of the densities.*/
	if ( !has_phase( model_phases , GAS + OIL))
	  util_exit("Could not find either GAS_DEN or OIL_DEN kewyords in restart files\n");
      }
    }
    
    
    
    if( ecl_file_has_kw( restart1_file , "RPORV") || ecl_file_has_kw( restart2_file , "RPORV") ){   
      rporv1_kw    = ecl_file_iget_named_kw(restart1_file, "RPORV", 0);
      rporv2_kw    = ecl_file_iget_named_kw(restart2_file, "RPORV", 0);
    } else {
      printf("Sorry, the restartfiles does not contain RPORV\n");      
      exit(1);
    }


    /* 
       OK - now it seems the provided files have all the information
       we need. Let us start extracting, and then subsequently using
       it.
    */
    {
      ecl_kw_type * oil_den1_kw = NULL ;  
      ecl_kw_type * oil_den2_kw = NULL ;
      ecl_kw_type * gas_den1_kw = NULL ;
      ecl_kw_type * gas_den2_kw = NULL ;
      ecl_kw_type * wat_den1_kw = NULL ;
      ecl_kw_type * wat_den2_kw = NULL ;
      ecl_kw_type * sgas1_kw 	= NULL;
      ecl_kw_type * sgas2_kw 	= NULL;
      ecl_kw_type * swat1_kw 	= NULL;
      ecl_kw_type * swat2_kw 	= NULL;
      ecl_kw_type * aquifern_kw = NULL ;

      /** Extracting the densities */
      {
	// OIL_DEN
	if( has_phase(model_phases , OIL) ) {
	  oil_den1_kw  = ecl_file_iget_named_kw(restart1_file, "OIL_DEN", 0);
	  oil_den2_kw  = ecl_file_iget_named_kw(restart2_file, "OIL_DEN", 0);
	}
	
	// GAS_DEN
	if( has_phase( model_phases , GAS) ) {
	  gas_den1_kw  = ecl_file_iget_named_kw(restart1_file, "GAS_DEN", 0);
	  gas_den2_kw  = ecl_file_iget_named_kw(restart2_file, "GAS_DEN", 0);
	}
	
	// WAT_DEN
	if( has_phase( model_phases , WATER) ) {
	  wat_den1_kw  = ecl_file_iget_named_kw(restart1_file, "WAT_DEN", 0);
	  wat_den2_kw  = ecl_file_iget_named_kw(restart2_file, "WAT_DEN", 0);
	}
      }
      

      /* Extracting the saturations */
      {
	// SGAS
	if( has_phase( file_phases , GAS )) {
	  sgas1_kw     = ecl_file_iget_named_kw(restart1_file, "SGAS", 0);
	  sgas2_kw     = ecl_file_iget_named_kw(restart2_file, "SGAS", 0);
	} 

	// SWAT
	if( has_phase( file_phases , WATER )) {
	  swat1_kw     = ecl_file_iget_named_kw(restart1_file, "SWAT", 0);
	  swat2_kw     = ecl_file_iget_named_kw(restart2_file, "SWAT", 0);
	} 
      }
      
      /* The numerical aquifer information */
      if( ecl_file_has_kw( init_file , "AQUIFERN")) 
	aquifern_kw     = ecl_file_iget_named_kw(init_file, "AQUIFERN", 0);
      
      {
	float * zero     = util_malloc( nactive * sizeof * zero     , __func__);    /* Fake vector of zeros used for densities / sturations when you do not have data. */
	int   * int_zero = util_malloc( nactive * sizeof * int_zero , __func__);    /* Fake vector of zeros used for AQUIFER when the init file does not supply data. */
	{
	  int i;
	  for (i=0; i < nactive; i++) {
	    zero[i]     = 0;
	    int_zero[i] = 0;
	  }
	}
	{
	  const float * sgas1_v   = safe_get_float_ptr( sgas1_kw 	  , NULL );
	  const float * swat1_v   = safe_get_float_ptr( swat1_kw 	  , NULL );
	  const float * oil_den1  = safe_get_float_ptr( oil_den1_kw , zero );
	  const float * gas_den1  = safe_get_float_ptr( oil_den1_kw , zero );
	  const float * wat_den1  = safe_get_float_ptr( oil_den1_kw , zero );
	  
	  const float * sgas2_v   = safe_get_float_ptr( sgas2_kw 	  , NULL );
	  const float * swat2_v   = safe_get_float_ptr( swat2_kw 	  , NULL );
	  const float * oil_den2  = safe_get_float_ptr( oil_den2_kw , zero );
	  const float * gas_den2  = safe_get_float_ptr( oil_den2_kw , zero );
	  const float * wat_den2  = safe_get_float_ptr( oil_den2_kw , zero );
	  
	  const float * rporv1    = ecl_kw_get_float_ptr(rporv1_kw);
	  const float * rporv2    = ecl_kw_get_float_ptr(rporv2_kw);
	  int   * aquifern;
	  int global_index;
	  
	  if (aquifern_kw != NULL)
	    aquifern = ecl_kw_get_int_ptr( aquifern_kw );
	  else
	    aquifern = int_zero;
	  
	  
	  for (global_index=0;global_index < nx*ny*nz; global_index++){
	    const int act_index = ecl_grid_get_active_index1( grid_file , global_index );
	    if (act_index >= 0) {
	      if(aquifern[act_index] < 1){ // Not numerical aquifer 
		float swat1 = swat1_v[act_index];
		float swat2 = swat2_v[act_index];
		float sgas1 = 0;
		float sgas2 = 0;
		float soil1 = 0;
		float soil2 = 0;
		
		truncate_saturation( &swat1 );
		truncate_saturation( &swat2 );
		
		if (has_phase( model_phases , GAS)) {
		  if (has_phase( file_phases , GAS )) {
		    sgas1 = sgas1_v[act_index];
		    sgas2 = sgas2_v[act_index];
		    truncate_saturation( &sgas1 );
		    truncate_saturation( &sgas2 );
		  } else {
		    sgas1 = 1 - swat1;
		    sgas2 = 1 - swat2;
		  }
		}
		
		if (has_phase( model_phases , OIL )) {
		  soil1 =  1 - sgas1  - swat1;
		  soil2 =  1 - sgas2  - swat2;
		  truncate_saturation( &soil1 );
		  truncate_saturation( &soil2 );
		}
		
		//printf ("SOIL1 : %f\t",soil1);
		//printf ("SGAS1 : %f\t",sgas1);
		//printf ("SWAT1 : %f\n",swat1);
		//
		//printf ("SOIL2 : %f\t",soil2);
		//printf ("SGAS2 : %f\t",sgas2);
		//printf ("SWAT2 : %f\n",swat2);
		//printf ("DEN_GAS1 : %f\n",gas_den1[act_index]);
		//printf ("DEN_GAS2 : %f\n",gas_den2[act_index]);
		
		//if(swat1[act_index] < 0.999 && swat2[act_index] < 0.999){	  // Check if this is an aquifer cell; neglect these
		

	
		/* 
		   We have found all the info we need for one cell, then we loop over all the grav
		   stations and calculate the delta G from this cell for the various stations.
		*/
		{
		  double  mas1 , mas2;
		  double  xpos , ypos , zpos;
		  int     station_nr;

		  mas1 = rporv1[act_index]*(soil1 * oil_den1[act_index] + sgas1 * gas_den1[act_index] + swat1 * wat_den1[act_index] );
		  mas2 = rporv2[act_index]*(soil2 * oil_den2[act_index] + sgas2 * gas_den2[act_index] + swat2 * wat_den2[act_index] );
		  //if(!(mas1>=0) || !(mas2>=0)){
		  //  printf("Cell index: %i  %i %i %i %i \n", act_index, global_index, i, j, k);
		  //  printf("mas1: %f mas2: %f %i\n", mas1, mas2, act_index);
		  //  printf("%f %f %f %f\n", rporv1[act_index], oil_den1[act_index], gas_den1[act_index], wat_den1[act_index] );
		  //  printf("%f %f %f %f\n", rporv2[act_index], oil_den2[act_index], gas_den2[act_index], wat_den2[act_index] );
		  //  exit(1);
		  //}
		  
		  
		  ecl_grid_get_pos1(grid_file , global_index , &xpos , &ypos , &zpos);
		  for(station_nr=0; station_nr < vector_get_size( grav_stations ); station_nr++) {
		    grav_station_type * g_s = vector_iget(grav_stations, station_nr);
		    double dist_x  = xpos - g_s->utm_x;
		    double dist_y  = ypos - g_s->utm_y;
		    double dist_d  = zpos - g_s->depth;
		    double dist_sq = dist_x*dist_x + dist_y*dist_y + dist_d*dist_d;
		    double ldelta_g;

		    if(dist_sq == 0){
		      exit(1);
		    }
		    ldelta_g = 6.67E-3*(mas2 - mas1)*dist_d/pow(dist_sq, 1.5);
		    grav_diff_update(g_s , ldelta_g);
		    //printf("DIST %f \n",dist_sq);
		    //printf ("DELTA_G: %f %f\n", g_s->grav_diff, ldelta_g);
		  }
		}
	      }
	    }
	  }
	}
	free( zero );
	free( int_zero );
      }
    }
    {
      int station_nr;
      for(station_nr = 0; station_nr < vector_get_size( grav_stations ); station_nr++){
	const grav_station_type * g_s = vector_iget_const(grav_stations, station_nr);
	printf ("DELTA_G: %f\n", g_s->grav_diff);
      }
    }
    
    // Clean up the mess 
    
    vector_free( grav_stations );
    ecl_grid_free(grid_file);
    ecl_file_free(restart1_file);
    ecl_file_free(restart2_file);
    ecl_file_free(init_file);
    free( grid_filename );
    free( init_filename );
    free( restart1_filename);
    free( restart2_filename);
  }		
}
