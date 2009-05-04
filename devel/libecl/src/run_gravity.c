#include <util.h>
#include <hash.h>
#include <stdio.h>
#include <string.h>
#include <ecl_file.h>
#include <ecl_util.h>
#include <vector.h>
#include <ecl_grid.h>


/*****************************************************************/
/**/


void grid_test(char * filename) {
  int nx,ny,nz,nactive;
  int global_index;
  ecl_grid_type * grid = ecl_grid_alloc(filename , true);  
  
  ecl_grid_get_dims( grid , &nx , &ny , &nz , &nactive);
  for (global_index = 0; global_index < nx*ny*nz; global_index++) {
    if (ecl_grid_cell_active1( grid , global_index)) {
      /* Ok this is an active cell - do your work ...*/
      double xpos,ypos,zpos;
      ecl_grid_get_pos1(grid , global_index , &xpos , &ypos , &zpos);
    }
  }    
  ecl_grid_free( grid );
}


//
//  Demonstration of ecl_util_alloc_filename();
//
//  char * restart_file = ecl_util_alloc_filename("/tmp" , "CASE3" , ECL_RESTART_FILE , false , 67);                                   => restart_file = "/tmp/CASE3.X0067";
//
//  char * egrid_file   = ecl_util_alloc_filename(NULL /* NO leading path */ , "CASE3" , ECL_EGRID_FILE , true , -1 /* Irrelevant *);  => egrid_file   = "CASE3.EGRID";
//




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
  s->utm_x =x;
  s->utm_y =y;
  s->depth =d;
  s->grav_diff=0.0;
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
    const char * file_path = argv[1];    
    const char * time1     = argv[2];    
    const char * time2     = argv[3];    
    const char * station_file = argv[4];    
    
    // Check if all the relevant files exist
    
    // *.GRID
    char * grid_filename = util_alloc_joined_string((const char *[3]) {file_path , "." , "GRID"} , 3 , "");
    if (!(util_file_exists(grid_filename))) {
      printf("Sorry, file:, %s does not exist", grid_filename);
      exit(1);
    }
    

    
    // *.XTIME1
    char * file1_ext = "NOPE";
    int r_index = atoi(time1);
    sprintf(&file1_ext, "%04d", r_index);
    char * restart1_filename = util_alloc_joined_string((const char *[3]) {file_path , ".X", &file1_ext} , 3 , "");
    if (!(util_file_exists(restart1_filename))) {
      printf("Sorry, file:, %s does not exist", restart1_filename);
      exit(1);
    }
    
    // *.XTIME2
    char * file2_ext = "NOPE";
    r_index = atoi(time2);
    sprintf(&file2_ext, "%04d", r_index);
    char * restart2_filename = util_alloc_joined_string((const char *[3]) {file_path , ".X", &file2_ext} , 3 , "");
    if (!(util_file_exists(restart2_filename))) {
      printf("Sorry, file:, %s does not exist", restart2_filename);
      exit(1);
    }
    
    // station_file
    if (!(util_file_exists(station_file))) {
      printf("Sorry, file:, %s does not exist", station_file);
      exit(1);
    }
    
    vector_type * grav_stations = vector_alloc_new();
    
    FILE * stream = util_fopen(station_file , "r");
    bool at_eof = false;
    int num_stations = 0;
    while(!(at_eof))
      {
	double x,y,d;
	int fscanf_return = fscanf(stream, "%lg%lg%lg", &x,&y,&d);
	if(fscanf_return ==3){
	  grav_station_type * g = grav_station_alloc(x,y,d);
	  vector_append_owned_ref(grav_stations, g, free);
	  num_stations++;
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
    

    
    // Allocate the files 
    ecl_file_type * grid_file     = ecl_file_fread_alloc(grid_filename , true);
    ecl_file_type * restart1_file = ecl_file_fread_alloc(restart1_filename , true);
    ecl_file_type * restart2_file = ecl_file_fread_alloc(restart2_filename , true);
    
    // Get the relevant kws
    
    ecl_kw_type * rporv1_kw    = ecl_file_iget_named_kw(restart1_file, "RPORV", 0);
    ecl_kw_type * rporv2_kw    = ecl_file_iget_named_kw(restart2_file, "RPORV", 0);
    
    ecl_kw_type * oil_den1_kw  = ecl_file_iget_named_kw(restart1_file, "OIL_DEN", 0);
    ecl_kw_type * gas_den1_kw  = ecl_file_iget_named_kw(restart1_file, "GAS_DEN", 0);
    ecl_kw_type * wat_den1_kw  = ecl_file_iget_named_kw(restart1_file, "WAT_DEN", 0);
    
    ecl_kw_type * oil_den2_kw  = ecl_file_iget_named_kw(restart2_file, "OIL_DEN", 0);
    ecl_kw_type * gas_den2_kw  = ecl_file_iget_named_kw(restart2_file, "GAS_DEN", 0);
    ecl_kw_type * wat_den2_kw  = ecl_file_iget_named_kw(restart2_file, "WAT_DEN", 0);
    		  			     
    ecl_kw_type * swat1_kw     = ecl_file_iget_named_kw(restart1_file, "SWAT", 0);
    ecl_kw_type * sgas1_kw     = ecl_file_iget_named_kw(restart1_file, "SGAS", 0);
    		  			     
    ecl_kw_type * swat2_kw     = ecl_file_iget_named_kw(restart2_file, "SWAT", 0);
    ecl_kw_type * sgas2_kw     = ecl_file_iget_named_kw(restart2_file, "SGAS", 0);
    		  			     
    //ecl_kw_type * dimens_kw    = ecl_file_iget_named_kw(grid_file, "DIMENS", 0);    
    
    ecl_kw_type * coord_kw;
    ecl_kw_type * corners_kw;
    
    // Get the relavant vectors
    float * rporv1   = ecl_kw_get_float_ptr(rporv1_kw);
    float * rporv2   = ecl_kw_get_float_ptr(rporv2_kw);
    
    float * oil_den1 = ecl_kw_get_float_ptr(oil_den1_kw);
    float * gas_den1 = ecl_kw_get_float_ptr(gas_den1_kw);
    float * wat_den1 = ecl_kw_get_float_ptr(wat_den1_kw);

    float * oil_den2 = ecl_kw_get_float_ptr(oil_den2_kw);
    float * gas_den2 = ecl_kw_get_float_ptr(gas_den2_kw);
    float * wat_den2 = ecl_kw_get_float_ptr(wat_den2_kw);
    
    float * swat1    = ecl_kw_get_float_ptr(swat1_kw);
    float * sgas1    = ecl_kw_get_float_ptr(sgas1_kw);
    
    float * swat2    = ecl_kw_get_float_ptr(swat2_kw);
    float * sgas2    = ecl_kw_get_float_ptr(sgas2_kw);

    
    // Fetch the coordiantes of the grid
    const int num_cells             = ecl_file_get_num_named_kw(grid_file, "COORDS");
    int ikw, i, j;
    int coord_size;
    //printf("The number of cells is: %i\n", num_cells);
    int act_index = 0;
    
    float soil1, soil2;
    double mas1, mas2;
    float coord_x, coord_y, coord_z;
    
    for (ikw=0;ikw<num_cells;ikw++){
      coord_kw     =  ecl_file_iget_named_kw(grid_file, "COORDS", ikw);
      coord_size   =  ecl_kw_get_size(coord_kw);
      if(coord_size <5){printf("The number of entries in t he coord_kw is less than 5, exiting\n");exit(1);}
      int * coord     = ecl_kw_get_int_ptr(coord_kw);
      
      if(coord[4] == 1){	// Active cell, let's go
	//printf("The indices of this active cell are IX: %i IY: %i IZ %i\n", coord[0], coord[1], coord[2]);
	//int index = coord[0] + n_i*(coord[1]-1) + n_i*n_j*(coord[2]-1);
	//printf("INDEX IS: %i and %i\n", coord[3], index);
	
	// Truncate the saturations
	if(sgas1[act_index] > 1.0){sgas1[act_index] = 1.0;};
	if(sgas1[act_index] < 0.0){sgas1[act_index] = 0.0;};
	if(sgas2[act_index] > 1.0){sgas2[act_index] = 1.0;};
	if(sgas2[act_index] < 0.0){sgas2[act_index] = 0.0;};

	if(swat1[act_index] > 1.0){swat1[act_index] = 1.0;};
	if(swat1[act_index] < 0.0){swat1[act_index] = 0.0;};
	if(swat2[act_index] > 1.0){swat2[act_index] = 1.0;};
	if(swat2[act_index] < 0.0){swat2[act_index] = 0.0;};
	
	// Calculate oil saturations
	soil1 = 1.0 - swat1[act_index] - sgas1[act_index]; 
	soil2 = 1.0 - swat2[act_index] - sgas2[act_index]; 
	
	if(soil1 > 1.0){soil1 = 1.0;};
	if(soil1 < 0.0){soil1 = 0.0;};
	if(soil2 > 1.0){soil2 = 1.0;};
	if(soil2 < 0.0){soil2 = 0.0;};

	//printf ("SOIL1 : %f\n",soil1);
	//printf ("SGAS1 : %f\n",sgas1[act_index]);
	//printf ("SWAT1 : %f\n",swat1[act_index]);
	
	if(swat1[act_index] < 0.999 && swat2[act_index] < 0.999){	  // Check if this is an aquifer cell; neglect these
	  
	  mas1 = rporv1[act_index]*(soil1 * oil_den1[act_index] + sgas1[act_index] * gas_den1[act_index] + swat1[act_index] * wat_den1[act_index] );
	  mas2 = rporv2[act_index]*(soil2 * oil_den2[act_index] + sgas2[act_index] * gas_den2[act_index] + swat2[act_index] * wat_den2[act_index] );
	  
	  // Calculate the cell center coordinate
	  corners_kw     =  ecl_file_iget_named_kw(grid_file, "CORNERS", ikw);
	  float * corners = ecl_kw_get_float_ptr(corners_kw);
	  coord_x = 0.0;
	  coord_y = 0.0;
	  coord_z = 0.0;
	  
	  for (i=0;i<24;i=i+3){
	    coord_x = coord_x + corners[i];  
	    coord_y = coord_y + corners[i+1];  
	    coord_z = coord_z + corners[i+2];  
	  }
	  
	  coord_x = coord_x / 8.0;
	  coord_y = coord_y / 8.0;
	  coord_z = coord_z / 8.0;
	  
	  // printf("Senter-koordinatene til cella er: %f %f %f\n",coord_x, coord_y, coord_z);
	  
	  for(j=0; j<num_stations;j++){
	    const grav_station_type * g_s = vector_iget_const(grav_stations, j);
	    double dist_x = coord_x - g_s->utm_x;
	    double dist_y = coord_y - g_s->utm_y;
	    double dist_d = coord_z - g_s->depth;
	    double ldelta_g = 6.67E-3*(mas2 - mas1)/(dist_x*dist_x + dist_y*dist_y + dist_d*dist_d);
	    grav_diff_update(g_s, ldelta_g);
	  }
	}	
	act_index++;
      }
    }
    
    for(j=0; j<num_stations;j++){
      const grav_station_type * g_s = vector_iget_const(grav_stations, j);
      printf ("DELTA_G: %f\n", g_s->grav_diff);
    }
    
    // Clean up the mess 
    
    vector_free( grav_stations );
    
    ecl_file_free(grid_file);
    ecl_file_free(restart1_file);
    ecl_file_free(restart2_file);
    
					     
  }		
}
